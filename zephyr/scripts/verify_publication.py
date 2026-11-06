"""Check publication contents and unchanged firmware without touching hardware."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import subprocess

COLLECTION = Path(__file__).resolve().parents[1]
REPOSITORY = COLLECTION.parent
EXPECTED_MEMORY = [(78156, 19176), (81364, 18464), (76724, 17136),
                   (101328, 20728), (81608, 19320)]


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-workspace', type=Path)
    parser.add_argument('--index', action='store_true', help='Also verify staged firmware bytes')
    args = parser.parse_args()
    manifest = json.loads((COLLECTION/'docs/publication_manifest.json').read_text(encoding='utf-8'))
    projects = sorted(p for p in COLLECTION.glob('0*') if p.is_dir())
    require(len(projects) == 5, 'Expected five numbered Zephyr applications')
    for relative, expected in manifest['files'].items():
        path = COLLECTION/relative
        require(digest(path) == expected, f'Firmware/configuration changed: {relative}')
        if args.index:
            staged = subprocess.run(['git', 'show', ':zephyr/'+relative], cwd=REPOSITORY,
                                    capture_output=True, check=True).stdout
            require(hashlib.sha256(staged).hexdigest() == expected,
                    f'Staged firmware/configuration differs: {relative}')
        if args.source_workspace:
            require(digest(args.source_workspace/relative) == expected,
                    f'Original workspace differs: {relative}')
    print(f"PASS: {len(manifest['files'])} firmware/configuration files match supplied hashes")
    reports = json.loads((COLLECTION/'docs/build_audit.json').read_text(encoding='utf-8'))
    require(len(reports) == 5, 'Expected five build audit records')
    for project, expected in zip(projects, EXPECTED_MEMORY):
        report = json.loads((project/'memory_report.json').read_text(encoding='utf-8'))
        require((report['flash_bytes'], report['ram_bytes']) == expected,
                f'Memory snapshot mismatch: {project.name}')
        require(report['hardware_verification'] == 'PENDING', 'Unexpected hardware claim')
        require(next(r for r in reports if r['project'] == project.name) == report,
                f'Consolidated audit differs: {project.name}')
        readme = (project/'README.md').read_text(encoding='utf-8')
        for marker in ('## Objective', '## RTOS concepts', '```mermaid', 'PENDING'):
            require(marker in readme, f'{project.name}: missing {marker}')
        if args.source_workspace:
            for extension, artifact in report['artifacts'].items():
                path = args.source_workspace/project.name/'build-verified'/'zephyr'/f'zephyr.{extension}'
                require(digest(path) == artifact['sha256'],
                        f'Original artifact fingerprint differs: {path.name}')
    print('PASS: five memory snapshots, evidence records and pending hardware status')

    # Resolve local links only; remote URLs and Mermaid rendering are not network-tested.
    markdown = [REPOSITORY/'README.md', REPOSITORY/'CONTRIBUTING.md',
                REPOSITORY/'THIRD_PARTY_NOTICES.md']
    markdown += list((REPOSITORY/'docs').rglob('*.md'))
    markdown += [f for f in COLLECTION.rglob('*.md')
                 if not any(p.startswith('build') for p in f.relative_to(COLLECTION).parts)]
    for file in markdown:
        text = file.read_text(encoding='utf-8')
        require(len(re.findall(r'^\s*```', text, re.M)) % 2 == 0,
                f'Unclosed Markdown fence: {file}')
        require('\ufffd' not in text, f'Invalid text encoding: {file}')
        for target in re.findall(r'\]\(([^)\n]+)\)', text):
            if target.startswith(('https:', 'http:', '#', 'mailto:')):
                continue
            path = target.split('#')[0]
            require((file.parent/path).exists(), f'Broken local link: {file.name}: {path}')
    for script in (COLLECTION/'scripts').glob('*.py'):
        ast.parse(script.read_text(encoding='utf-8'), filename=str(script))
    print('PASS: Markdown fences/local links and Python syntax')

    legacy = subprocess.run(['git', 'ls-tree', '-r', '--name-only',
                             manifest['repository_base_commit']], cwd=REPOSITORY,
                            capture_output=True, text=True, check=True).stdout.splitlines()
    protected = [p for p in legacy if p not in ('README.md', '.gitignore')]
    result = subprocess.run(['git', 'diff', '--exit-code', manifest['repository_base_commit'],
                             '--', *protected], cwd=REPOSITORY, capture_output=True)
    require(result.returncode == 0, 'Existing license or unrelated repository file changed')
    print('PASS: existing license, RGB application and SDK documentation unchanged')

    forbidden = {'.elf', '.hex', '.bin', '.map', '.o', '.obj', '.pem', '.key'}
    secrets = re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|'
                         r'gh[pousr]_[A-Za-z0-9_]{20,}|github_pat_[A-Za-z0-9_]{20,}')
    for file in COLLECTION.rglob('*'):
        if not file.is_file() or any(p.startswith('build') or p == '__pycache__'
                                     for p in file.relative_to(COLLECTION).parts):
            continue
        require(file.suffix.lower() not in forbidden, f'Unwanted artifact: {file.name}')
        require(not secrets.search(file.read_text(encoding='utf-8')),
                f'Credential-like content requires review: {file.name}')
    print('PASS: publication files exclude binaries and detected credential patterns')
    print('Hardware flashing, UART observation and runtime measurements: PENDING; not performed')


if __name__ == '__main__':
    main()
