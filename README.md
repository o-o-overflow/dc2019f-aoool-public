# AOOOL

This repo hosts the source and sample exploits for the AOOOL service of DEF CON 27 CTF Finals.

AOOOL is a simple web server written in C/C++.

The service had the following features:
- Simple webserver supporting well-formed HTTP GET requests
- Possibility of uploading files via the UF (Upload File) HTTP method
- Configuration via a nginx-like (simplified) configuration format
- Possibility of updating the config at run-time via the UC (Update Config) HTTP method
- Support for OSL (OOO Scripting Language), a custom JITted language developed for this service

## Notes on config

- There are three nodes: root, server, and location.
- server nodes are matched via the `server_name` directive
- location nodes are matched via the specified pattern, e.g., `location "\*.gif" { ... }`
- for each node, you can specify:
  - `root`: root path from where files should be served from
  - `log`: file path for logs
  - `mode`: this can be `text` (for normal files) and `osl` for OSL scripts
- see [default config](./service/deployment/files/aoool/etc/default) as an example

## Notes on OSL

OSL was the juicy part of the service.

Main features of OSL:
- it has been written from scratch: the aoool service contains the scanner/parser, code for building AST, the jitter (which goes through the AST nodes and emits machine code, on the fly), and code to execute the generated code
- basic integer and string operations
- variable assignment (and deletion)
- print variable statement (with no parenthesis, dedicated to all python2 fans ;-))
- "print log" statement
- it uses a dedicated area for `.text` and `.data`, and a dedicated stack
- it stores the real stack pointer in a dedicated metadata section

## Notes on bugs

There were several intended (and I guess unintended) bugs. Some of the intended bugs are:
- several type confusion bugs in the jitter (see comments in the code emitter: [osleval.h](./service/src/osleval.h)).
- buffer overflow when writing the jitter code in memory
- possible stack clashing (by evaluating a "deep" expression)
- buggy variable => memory mapping
- path traversals via config files
- logic bug in how relative paths are handled for the logs (i.e., config file parsing use `/aoool/var/log` as root dir, while the jitter uses `/`)

## Notes on exploitation

- Using the bugs above the attacker can achieve various forms of arbitrary read and write + possibility of leaking address of the jitted memory
- Possible exploitation strategy: leak area of jitted memory (via type confusion), use arbitrary write to write a shellcode in the RWX memory, arbitrary write to overwrite the saved stack pointer, pivot stack + `$rip` control to shellcode
- log-related logic bug exploitation: 1) upload config with log pointing to `flag` (this is treated as a relative path and it passes the check as the service thinks it points to `/aoool/var/log/flag`), 2) execute OSL containing `print log`.
- See [exploit.py](./remote-interaction/exploit.py) for a couple of examples.
