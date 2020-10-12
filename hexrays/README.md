## *Transplanting* Hex-Rays Decompiler between IDA versions

*DISCLAIMER*: Apparently I cannot upload the patched binary and/or the original ones without [having GitHub yell at me](https://github.com/github/dmca/blob/master/2018/2018-04-05-HexRays.md) to [take it down](https://github.com/github/dmca/blob/master/2019/09/2019-09-12-Hex-Rays.md), so only binary diffs are provided.

### Hex-Rays 7.2 for IDA 7.3

Made x64 decompiler (`hexx64.dll`) from IDA Pro 7.2 compatible with IDA Pro 7.3.

![We all like proof of concept screenshots](images/hexx64_7.2_7.3.png)

Binary diff in <hexx64_7.2_7.3.txt>.

*TODO: Description on what has been done*

### Hex-Rays 7.5 Demo for IDA 7.3/7.2/7.0?

btw, I've also tried to port x64 decompiler (`hexx64.dll`) from IDA Demo 7.5 downwards. Guess what? I made it, and of course, without (most of) the demo limitations.

Limitations that are still there:

- Some functions have their code removed, leaving a "full version only" message:
	- "Jump to xref globally": `hx:JumpGlobalXref`, Ctrl-Alt-X
	- "Create C file": `hx:CreateCFile`, Ctrl-F5, "File"-"Produce file"-"Create C file..."
	- "Extract function": `hx:ExtractFunc`, None, "Help"-"Extract function"
- ...

![IDA Pro 7.3 was the original goal](images/hexx64_7.5_7.3.png)

![But it works with IDA Pro 7.2 too](images/hexx64_7.5_7.2.png)

IDA Pro 7.0 *might* work, but I have not tested it yet.

Binary diff in <hexx64_7.5_7.x.txt>. Shoutout to [ericyudatou@bbs.pediy.com](https://bbs.pediy.com/thread-262435.htm), without your hard work this patch would be impossible.

*TODO: Description on what has been done*

**UPDATE 2020-10-12**: Fixed a bug causing IDA to crash when decompiling a "varargs" function on IDA 7.2.

