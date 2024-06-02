# Media Files for Testing the Sample Encoder and Reference Decoder

The test images and results of applying the sample encoder and reference decoder to the test images
are organized into subdirectories for each type of test image.

[boxes/](./boxes)
> Test images consisting of solid color boxes with random locations, dimensions, and colors.
The hard edges in the boxes are good for detecting errors in the wavelet transforms.

[gradient/](./gradient)
> Test images with a gradual gradient.

[solid/](./solid)
> Test images with a constant color that is randomly chosen.

[sections/](./sections)
> Images for testing VC-5 layers and sections (ST 2073-5 and ST 2073-6).

[metadata/](./metadata)
> Test cases for verifying conformance with ST 2073-7 (see RP 2073-2 for details).


The subdirectories that contain test images are divided into a hierarchy of image dimensions
and pixel format.

In each subdirectory containing test images, there is a master subdirectory that is itself
subdivided into subdirectories for each part of the VC-5 standards suite. For each part in
the VC-5 standards suite (except part 7), there are subdirectories for each build configuration.
Each build configuration contains bitstreams produced by the sample encoder and images output
by the reference decoder. The image test case, dimensions, pixel format, and VC-5 part are
represented in the filename.

Images are stored in a unformatted binary file that contains only the image raster.
The filename contains the information that would be in an image header: dimensions and pixel format.
No existing file format can contain all of the pixel formats used for testing the VC-5 codec.

