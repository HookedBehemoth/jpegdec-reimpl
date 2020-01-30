PFS0S := jpegdec

SUBFOLDERS := Atmosphere-libs $(PFS0S)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(PFS0S): Atmosphere-libs

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)