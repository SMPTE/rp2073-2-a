---
title: Layer Descriptions
---

# Introduction

This white paper suggests a format for the description string in layer metadata.

The intent is to use a format for the string that is machine and human readable.

The recommendation is to adapt kay-value pairs from YAML [YAML Syntax][].
A key value pair is a string without quotes, a colon, and a string (also without quotes). The layer description can be a string of key-value pairs separated by commas.

For example,

"exposure value: 1/60, aperture: f/1.4"

Note that the key in a YAML key-value pair can contains spaces [YAML Keys][].



# References


[YAML Syntax]: https://docs.ansible.com/ansible/latest/reference_appendices/YAMLSyntax.html

[YAML Keys]: https://stackoverflow.com/questions/40977183/spaces-in-yaml-keys

