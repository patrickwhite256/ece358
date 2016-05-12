Peers
=====

$ addpeer
- should output host, port, and create process

$ addpeer
(out)
$ removepeer (out)
- should have no output
- should cause process created by first to die

$ addpeer 1.2.3.4 1111
- should error

$ addpeer
(out)
$ addpeer (out)
- should output host, port
- there should now be two processes running

$ removepeer
- should error

Content (functionality only)
============================

$ addcontent 1.2.3.4 1111 "content"
- should error

$ addpeer 
(out)
$ addcontent (out) "content"
- should output key

$ addpeer
(out)
$ removepeer (out)
$ addcontent (out) "content"
- should error


