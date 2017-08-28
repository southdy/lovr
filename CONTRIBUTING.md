Contributing
===

You want to contribute to LÖVR?  That's awesome!

Submitting Issues
---

Feel free to file an issue if you notice a bug.  Make sure you search before filing an issue, it may
have already been asked!

Issues are okay for feature requests and questions about the development of LÖVR as well, but
usually you'll get a better response by asking in [Slack](https://join.slack.com/ifyouwannabemylovr/shared_invite/MTc5ODk2MjE0NDM3LTE0OTQxMTIyMDEtMzdhOGVlODFhYg).  Quesetions about using LÖVR belong
in Slack.

Contributing Documentation
---

If you see any typos or inconsistencies in the docs, submitting an issue or pull request in the
[lovr-docs repo](https://github.com/bjornbytes/lovr-docs) would be greatly appreciated!  There,
each documentation page is a markdown file in the `docs` folder, and examples can be found in the
`examples` folder.

Contributing Code
---

To contribute patches, fork LÖVR, commit to a branch, and submit a pull request.  For larger
changes, it can be a good idea to engage in initial discussion via issues or Slack before
submitting.  Try to stick to the existing coding style:

- 2 space indentation.
- 100 character wrapping (ish, sometimes it's more readable to just have a long line).
- Use descriptive, camelCased names when possible.

If you modify the embedded `boot.lua` script, you can compile it into a C header by doing this:

```sh
pushd src/data; xxd -i boot.lua > boot.lua.h; popd
```