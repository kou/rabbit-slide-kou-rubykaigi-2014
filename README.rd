= Three Ruby usages

This talk describes three Ruby usages:

  * Implementing high-level interface by Ruby.
  * Using Ruby as a glue language.
  * Embedding Ruby into a C program for flexibility.

All of them have pros and cons. They are trade-off. If a case can
ignore cons, we will have pros for the case.

Most of Rubyists implement their Ruby applications only by Ruby. This
talk shows another options to use Ruby as your convenient tool. If you
know another options and trade-offs of them, you will implement your
Ruby applications more effectively.

This talk uses Droonga, a distributed full-text search engine, as a
sample application to describe these Ruby usages. Droonga uses these
three Ruby usages.

== License

=== Slide

CC BY-SA 4.0

Use the followings for notation of the author:

  * Kouhei Sutou

=== Images

==== ClearCode Inc. logo

CC BY-SA 4.0

Author: ClearCode Inc.

It is used in page header and some pages in the slide.

==== Ruby logo

File:

  * images/ruby.pdf

CC BY-SA 2.5

Author: Yukihiro Matsumoto

==== Groonga logos

Files:

  * images/groonga-logo.svg
  * images/rroonga-logo.svg
  * images/droonga-logo.svg

CC BY 3.0

Author: The Groonga Project

==== Images that uses Groonga logo

Files:

  * images/droonga-rroonga.svg
  * images/rroonga-search.svg

CC BY 3.0

Author:

  * Kouhei Sutou
  * The Groonga Project

==== Images that uses Ruby logo

Files:

  * images/embed.svg
  * images/high-level-interface.svg
  * images/high-level-interface-examples.svg

CC BY-SA 2.5

Author:

  * Kouhei Sutou
  * Yukihiro Matsumoto

==== Images that uses Groonga logo and Ruby logo

Files:

  * images/droonga-mruby.svg
  * images/embed-examples.svg

CC BY-SA 2.5

Author:

  * Kouhei Sutou
  * Yukihiro Matsumoto
  * The Groonga Project

==== Other images

Files:

  * images/clear-code-is-silver-sponsor.png
  * images/distributed-full-text-search-engine.svg
  * images/distributed-full-text-search-engine-components.svg
  * images/droonga-messaging-system.svg
  * images/glue-examples.svg
  * images/glue.svg
  * images/range-search-optimized.svg
  * images/range-search-simple.svg

CC BY-SA 4.0

Author: Kouhei Sutou

== For author

=== Show

  rake

=== Publish

  rake publish

== For viewers

=== Install

  gem install rabbit-slide-kou-rubykaigi-2014

=== Show

  rabbit rabbit-slide-kou-rubykaigi-2014.gem

