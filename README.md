# Directory/File Compare (dc4w) Project

#### History (Brief)

This is a **Windows ONLY** project.

An application to graphically compare two folders, or files, is **VERY IMPORTANT** to programmers... and perhaps equally important to HTML authors, and many, many others...

Some of the original source is from a 1992-2010 Microsoft windows support tools SDK, called `WinDiff`. Searching around I again found a zip of that source - see `windiff\zip\windiff_src.zip`. Just for fun, unzipped this source, move the files around a little, and massaged it to compile with MSVC 16 2019, first in 32-bits, then again in 64-bits... see [windiff/README.md][1] ... this is presented for **archival purposes only**.

But starting circa 2001, here it has been developed, with some of my own extensions, tested in 32-bit and 64-bit, and seem quite stable, all the source is in `src`... 

This repository takes over from my [Directory Compare (DC4W)][2] web page, where the **OLD** source was only available as a `ZIP` download...

That is, any further development, if there is any, will only be in this repository.

   [1]: windiff/README.md
   [2]: http://geoffair.com/ms/dc4w.htm

#### Licence (None)

To the degree possible, this source is released into the Public Domain. See the [COPYING.txt][10]


   [10]: COPYING.txt
  
#### Building

To compile `Dc4w` from source, you need [CMake][20] installed. This allows you to generate the build system of your choice. Run `cmake --help` to see the list of `Generators` available, on your system, and show the default generator.

If your chosen `generator` is installed, then to build `Dc4w` do -

```
   cd build
   cmake .. [options]
   cmake --build . --config Release
```

If cmake does not choose the desired build generator by default, the option `-G "Name of generator"` must be given. 
   
There are some `option` listed in the `CMakeLists.txt` file. Use `-DUSE_STATIC_RUNTIME=OFF`, to use the dynamic windows runtime - make a smaller executable.

Alternatively, you can use the cmake GUI. Navigate to where you put the source, and use the `build` folder, to where to build the binaries, then `Configure`, `Generate`, ...

And if all successful, and you use [MSVC][21], you can load the MSVC IDE, loading the `dc4w.sln`, and proceed to build the project from within the IDE...
   
In Windows, it does **not** make much sense to actually `install` the resultant `dc4w.exe`. You can create a Windows `shortcut` to the exe, to run it... or copy it to a folder already existing in your `PATH` environment variable, if you want to load it from the command line.

   [20]: http://www.cmake.org/install/
   [21]: https://visualstudio.microsoft.com/vs/community/

#### Future

After building, and using this app for **many** years, there are now better, more featured apps available -

   * [WinMerge][30] - A free project, with binary distribution, updates, and source available...  
   * [Beyond Compare][31] - A commercial app, but well worth the price... 
   * And there are MANY others... and if you need to compare folders remotely, then there is -   
   * [FileZilla][32] - A free project, with binary distribution, and automatic updates...  
   * [WinSCP][33] - Not personally tested, but looks good from the reading... must try it...  
   
And there are probably others... These days, most of these offer much more than my `dc4w`...
   
   [30]: https://winmerge.org/
   [31]: https://www.scootersoftware.com/
   [32]: https://sourceforge.net/projects/filezilla/
   [33]: https://winscp.net/eng/index.php

Anyway, have **FUN** - Geoff - 20200624
   
; eof


