# AllRGB

Takes an input image and creates an output image that approximates the input image by using every 24-bit colour exactly once. Output image is a square of dimension 4096x4096 = 256x256x256 pixels -- exactly one pixel per colour.

# Command Line
java -XX:-UseGCOverheadLimit -jar allrgb.jar path/to/original/image.png path/to/output/image.png
