GEMINI:// FUSE3 MOUNT COMMAND
===================================

Mount the "gemini://" global web as a file system in UNIX machines.

Example:

    # mkdir -p /www/gemini:
    # mount.gemini /www/gemini:
    # find /www/gemini:/geminiprotocol.net
    /www/gemini:/geminiprotocol.net
    /www/gemini:/geminiprotocol.net/index.gmi
    /www/gemini:/geminiprotocol.net/news
    /www/gemini:/geminiprotocol.net/news/index.gmi
    /www/gemini:/geminiprotocol.net/news/atom.xml
    /www/gemini:/geminiprotocol.net/news/2024_10_24.gmi
    /www/gemini:/geminiprotocol.net/news/2024_09_08.gmi
    /www/gemini:/geminiprotocol.net/news/2024_08_28.gmi
    ...
    /www/gemini:/geminiprotocol.net/docs
    /www/gemini:/geminiprotocol.net/docs/index.gmi
    /www/gemini:/geminiprotocol.net/docs/faq.gmi
    /www/gemini:/geminiprotocol.net/docs/gemtext.gmi
    ...
    /www/gemini:/geminiprotocol.net/history
    /www/gemini:/geminiprotocol.net/history/index.gmi
    /www/gemini:/geminiprotocol.net/history/phlog
    /www/gemini:/geminiprotocol.net/history/phlog/index.gmi
    ...
    /www/gemini:/geminiprotocol.net/software
    /www/gemini:/geminiprotocol.net/software/index.gmi
    /www/gemini:/geminiprotocol.net/software/http

## Building

You need libressl and fuse development files.

    $ sudo apt-get -y install libfuse3-dev libressl-dev
    $ make
    $ sudo make install

## Things to do

- Implement a pool in "pool.c" for making it faster.
- Add options for different parameters.
- Add symbolic links.
- Add gopher support.
- Add support for other URIs outside gemini, for example 
  `ftp://www.example.com name` would be a link named "name" linking
  to `/www/ftp:/www.example.com` where `/www/ftp:` is another file
  system.

## Style guide

This project follows the OpenBSD kernel source file style guide (KNF).

Read the guide [here](https://man.openbsd.org/style).

## Collaborating

For making bug reports, feature requests and donations visit
one of the following links:

1. [gemini://harkadev.com/oss/](gemini://harkadev.com/oss/)
2. [https://harkadev.com/oss/](https://harkadev.com/oss/)
