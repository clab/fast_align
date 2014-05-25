fast_align
==========

`fast_align` is a simple, fast, unsupervised word aligner.

If you use this software, please cite:
* [Chris Dyer](http://www.cs.cmu.edu/~cdyer), [Victor Chahuneau](http://victor.chahuneau.fr), and [Noah A. Smith](http://www.cs.cmu.edu/~nasmith). (2013). [A Simple, Fast, and Effective Reparameterization of IBM Model 2](http://www.ark.cs.cmu.edu/cdyer/fast_valign.pdf). In *Proc. of NAACL*.

The source code in this repository is provided under the terms of the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0.html).

`fast_align` is included in the [`cdec` translation system](http://www.cdec-decoder.org/). It uses the same model and produces identical alignments, but it has a few extra features.

# Input format

Input to `fast_align` must be tokenized and aligned into parallel sentences. Each line is a source language sentence and its target language translation, separated by a triple pipe symbol (`|||`). An example is as follows.

    doch jetzt ist der Held gefallen . ||| but now the hero has fallen .
    neue Modelle werden erprobt . ||| new models are being tested .
    doch fehlen uns neue Ressourcen . ||| but we lack new resources .

# Compiling and using `fast_align`

`fast_align` requires only a C++ compiler; it can be compiled by typing `make` at the command line prompt.

Run `fast_align` to see a list of command line options. Here is an example invocation:

    ./fast_align -i text.fr-en -d -o -v > forward.align

# Output

`fast_align` produces outputs in the `i-j` "Pharaoh" format, where a pair `i-j` indicates that the <i>i</i>th word of the source is aligned to the <i>j</i>th word of the target sentence. For example, a good alignment of the above example corpus would be:

    0-0 1-1 2-4 3-2 4-3 5-5 6-6
    0-0 1-1 2-2 2-3 3-4 4-5
    0-0 1-2 2-1 3-3 4-4 5-5


