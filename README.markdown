Searchlight
-----------
Searchlight is a refactored version of the early experiment [sphinx_views][1],
an attempt to build a views-driven search implementation specific to Sphinx.
Searchlight claims to be able to use search backends in a pluggable and/or
interchangeable manner, but currently there is only the Sphinx backend, so it's
all smoke & mirrors until I get a chance to put some Solr/Lucine action behind
it and see how well my design decisions worked out.


Goals
-----
Searchlight is rather different from any of the existing advanced tech search
implementations in Drupal out there currently. In no particular order, some
goals that set it apart:

- Each Drupal site should be able to define its own schema and index
  configuration. We're Views-centric, not node-centric, so anything that has
  been properly exposed to Views should be fair game.
- We think there is a lot of common ground between good search solutions, enough
  so that there can be a lot of shared interface points. We want search backends
  to be as swappable as possible, to the point where you can take a site that's
  using backend A and switch to backend B without rebuilding your Views, etc.


Status
------
This is not ready for deployment. It's hardly ready for collaborative
development. If you're reading this it should be because you're curious and are
willing to get burned, not because you want to actually use this code.


Terminology
-----------
- A **backend** is a CTools plugin class that interfaces with a specific search
  technology.
- A **datastore** is a backend-agnostic representation of a search datastore
  (aka index). In Searchlight, there are specific Views displays which help you
  represent each base table (and joining tables) as a datastore.
- A **field** is a data element exposed to the search backend. Each field is
  referenced by the **views alias** generated for it in its datastore view, and
  contains additional metadata like: datatype (int, text, timestamp, etc.),
  whether it should be exposed as a facet or not, etc. Each datastore contains
  definitions of one or more fields.


The backend interface
---------------------
The backend is a base class that defines the methods each implementing backend
must provide. The methods fall roughly into these categories:

- **General settings management.** Form and default settings methods are
  available for each backend and are saved to a drupal variable per backend
  through a system settings form. These are for generic sitewide settings, e.g.
  what port to use to connect to the Sphinx daemon.
- **Views handler settings.** Each backend may have particular quirks.
  Searchlight provides a single "meta" views filter handler and argument handler
  that calls the backend's settings to provide backend-specific options. You
  should provide sensible default values in case someone switches between
  backends on the fly -- if you have good defaults, everything should continue
  to work.
- **Views execution & query translation.** The base backend class provides the
  main views interface point for result set replacement and query object
  translation. This is where the magic happens and hopefully implementers will
  never have to override this logic. An example of what happens here - I can
  take a `WHERE table.aliased_field IN ('%s', '%s')` clause on the Views query
  object, and translate it into parameters for the search backend like
  `->setFilter($field, $operator, $args)`.
- **Search query methods.** These are the main methods the backend is
  responsible for implementing. They include things like: queryInit(),
  queryExecute(), setFilter(), setSort().
- **Facet builiding, rendering.** Facets represent a group of user-story driven
  queries (generally groupby's with a certain sort, like count desc or timestamp
  desc) and interface patterns. Backends will need to implement the query logic
  but ideally the UI and rendering methods can be left alone.


[1]: http://github.com/yhahn/sphinx_views