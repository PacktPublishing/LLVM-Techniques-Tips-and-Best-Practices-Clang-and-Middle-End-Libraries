# How To Use
The sample code here is presented in the form of patches
(since they need to be put inside the LLVM code base). Please apply than to
the llvm-project repository via `git apply`.

For instance:
```
git apply Base-CompilerRT.diff
```

If there are error messages showing conflict with the tree, it's possible that
the code base has been quite different since these code were developed. You can
choose to "rewind" to the time point when these two patches were created by using
the following command:
```
git checkout d11d5d1c5f5a8bafc28be98f43c15a3452abb98b
```
After that you can execute the `git apply` command again.

# About the Patches
In this sample project, we touched three sub-projects in LLVM: Clang, Compiler-RT and LLVM itself.
Not all the changes in each of them are really...interesting. So we split the patches for each sub-project
into two parts: The patch starts with `Base` (e.g. `Base-CompilerRT.diff`) contains supporting changes that
are optional in terms of the book content. On the other hand, patch file starts with `Changes` contains core
knowledges that are metnioned in the book.

For optimal experiences when you progress through this chapter, it is recommended to apply patches here
in the following ordering:
 1. Changes-LLVM.diff
 2. Base-CompilerRT.diff
 3. Changes-CompilerRT.diff
 4. Base-Clang.diff
 5. Changes-Clang.diff

