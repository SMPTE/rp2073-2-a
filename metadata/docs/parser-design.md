---
title: XML Parser Design Documentation
---

# Introduction


# Design

For each field in a metadata tuple define a function that takes the value from the tuple read from the XML file (if any) and returns the value to write into the bitstream.

For example:
```C
TAG bitstream_tag(TAG tag);
SIZE bitstream_size(TYPE type, COUNT count, SIZE size);
```

The idea is to encapsulate the logic for computing a bitstream field from other fields in the tuple if the field is not explicitly represented in the XML tuple read from a file.

