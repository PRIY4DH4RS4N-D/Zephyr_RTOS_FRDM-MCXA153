# License and dependency notices

The existing root [LICENSE](LICENSE) is unchanged: BSD 3-Clause, copyright 2026 PRIY4DH4RS4N-D. Repository-authored applications, documentation and scripts are distributed under that license.

NXP-supplied files in `01_RGB_LED_Controller/`, including generated board, pin and clock support, retain their embedded copyright and SPDX notices. The root license does not replace those notices. The external MCUXpresso SDK and toolchain are not redistributed.

The five Zephyr application source/configuration sets are copied unchanged from the supplied development workspace. Their [publication manifest](zephyr/docs/publication_manifest.json) records SHA-256 fingerprints. Common application helpers remain local to each independent application.

Zephyr 4.3.0, CMSIS, CMSIS_6 and HAL_NXP are external west dependencies, not vendored source trees. They retain their own upstream licenses, including Apache-2.0 and component-specific notices. External Python packages, CMake, Ninja, ARM GNU tools and NXP LinkServer retain their own license terms.

The [environment notes](zephyr/docs/zephyr_installation.md) list pinned revisions and versions. Memory/audit reports and build-analysis transcripts are engineering evidence; they do not represent hardware certification. Generated firmware binaries are not published in Git.
