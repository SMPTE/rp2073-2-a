# Make file for creating intrinsic metadata test cases (SMPTE ST 2073-7 Annex B)


# Categories of intrinsic metadata with each category in its own subdirectory
INTRINSIC = simple nested complex


all verify clean clean-all:
	@for d in $(INTRINSIC); do make -C $$d -f $$d.mk $@; done

