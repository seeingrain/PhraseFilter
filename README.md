# PhraseFilter
A high performance library for filtering sensitive words, UTF-8 encoding only, sensitive words will be replaced by '*'

Author: Richard Zhang.
Mail: 89205975@qq.com

This library filters sensitive phrases by user's configuration.
Currently, only support UTF8 & ANSI encoded strings.

The matching rule is max-length-matching, the library tries to match sensitive phrase as long as possible.
  For example:
  "damn fucker" and "damn" are all in sensitive dictionary, the sentence "he's a damn fucker" will be processed to "he's a ***********".

Even user insert some spaces or non-letter characters between sensitive words, the library is also able to deal with it.
  For example:
  "Bad boy" is added to sensitive dictionary, "Bad.boy", "Bad     boy", "Bad/boy" can also be filtered.
  "你去死" is added to sensitive dictionary, "你 去    死", "你/去    死", "你 去 .死" can also be filtered.



Compiling requirement:
  1. STL C++11
  2. BOOST multi_index_container



Performance test condition:
  1. Giving a sentence around 100 bytes (English & Chinese mixed)
  2. Dirty phrases around 10,000
  3. Do 1,000 loop test
  4. Intel I7 CPU

Test result:
  For each loop, it cost around 100us
