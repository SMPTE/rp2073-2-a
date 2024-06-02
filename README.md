# VC-5 Reference Decoder and Sample Encoder

SMPTE code repository for the VC-5 reference decoder and sample encoder.

The VC-5 software project is maintained by the
[VC-5 Mezzanine Compression Drafting Group](https://kws.smpte.org/higherlogic/ws/groups/bcf3dc28-fb2d-4d7e-90e1-d8c7d04ee223)
under
[TC-10E](https://kws.smpte.org/communities/community-home?CommunityKey=f6ed3b1a-3f1c-4684-9202-3794e6836a58).

The release notes are in [](./notes/release.md) and instructions for building and running this software
are in [](./notes/install.md). The release notes and build instructions can be converted to PDF and HTML formats
by running
```bash
make notes
```
in the root directory.

Subdirectories of the root directory:

[Archive/](./Archive)
> Obsolete files that will be removed from the repository.

[common/](./common)
> Source code and include files used by the encoder, decoder, and other programs.

[comparer/](./comparer)
> Source code for a tool that computes the PSNR difference between two images.

[converter/](./converter)
> Source code for a tool that converts images to other formats used by the reference codec.

[decoder/](./decoder)
> Source code for the reference decoder.

[encoder/](./encoder)
> Source code for the sample encoder.

[external/](./external)
> Software from other source code repositories used by the VC-5 programs in this repository.

[licenses/](./licenses)
> Overview of the licenses for external software currently used in the VC-5 codebase and copies
of the license agreements.

[media/](./media)
> Media files for testing the sample encoder and reference decoder.

[metadata/](./metadata)
> Programs and scripts for generating and managing the test cases used for verifying conformance
to ST 2073-7. See the [README](./metadata/README.md) file in the metadata subdirectory for details.

[notes/](./notes)
> Release notes and instructions for building the VC-5 software.

[scripts/](./scripts)
> Scripts for testing and managing the VC-5 software.
See the [README](./scripts/README.md) file in the scripts subdirectory for details.

[tables/](./tables)
> Codebooks used by the VC-5 codecc.

[Makefile](./Makefile)
> Master make file for building all VC-5 software and documentation.

Most of the subdirectories listed above contain a README file providing details on the component.

