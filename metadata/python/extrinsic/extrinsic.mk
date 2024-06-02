# Make file for creating extrinsic metadata test cases

# Categories of extrinsic metadata with each category in its own subdirectory
EXTRINSIC = xmp dpx mxf aces ale dmcvt


all verify clean clean-all:
	@for d in $(EXTRINSIC); do make -C $$d -f $$d.mk $@; done

