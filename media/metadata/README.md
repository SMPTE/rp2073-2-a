# Metadata Test Cases

Each of the subdirectories in this directory correspond to tables in RP 2073-2 Annex D.

input/
> Metadata test cases in XML format.

bitstreams/
> VC-5 bitstreams created by the sample encoder with embedded metadata. The image file
`$(ROOT)/media/boxes/1280x720/rg48/boxes-1280x720-0000.rg48` was used to create all of the
bitstreams.

> The bitstream file name is the same as the metadata test case in the `input` subdirectory
but with the file extension changed to `vc5`.

encoded/

> Binary file containing the encoded metadata extracted from the bitstream created by the
sample encoder.

decoded/
> Metadata in XML format output by the reference decoder.

