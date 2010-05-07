Installing Searchlight
----------------------
Installing Searchlight involves several steps, some going beyond the boundaries
of your Drupal site. You will need shell access and administrative-level
permissions on your server or local machine. Expect additional pain and
suffering if you are using Windows.

Overview:

1. Ensure basic requirements are met and install Drush.
2. Choose a search backend service and install it.
3. Install Searchlight on your Drupal site and configure its backend.
4. Configure the datasource(s) for your website.
5. Generate configuration files for your search service.
6. Index your content and start your search backend service.
7. Search for content.


Requirements & dependencies
---------------------------
- PHP 5.2 or greater
- Drupal 6.x or equivalent core (e.g. Pressflow)
- CTools 1.3 or later
- Views 2.x (a relatively recent release) or 3.x-alpha3
- PURL 1.x
- Drush 3.0
- One of the Search backend services (e.g. Apache Solr or Sphinx)


1. Ensure basic requirements are met and install Drush
------------------------------------------------------
Make sure you have all the required components and have installed and tested
Drush. Note that you will also need the respective requirements of the search
service you plan on using. For example, Sphinx requires standard UNIX build
tools as well as MySQL or PostgreSQL as your datasource. See the documentation
for Sphinx and/or Solr for more information:

- [Sphinx documentation](http://sphinxsearch.com/docs)
- [Solr Wiki](http://wiki.apache.org/solr)


2. Choose a search backend service and install it
-------------------------------------------------
Searchlight currently support Sphinx and Apache Solr. You only need to install
one of these search services. Searchlight cannot use both Solr and Sphinx at
the same time, though you can have both services installed and switch between
them during development. Instructions for installing each are below.

- Searchlight has been tested against [Sphinx 0.9.9][1]. Run the usual
  `.configure`, `make` and `make install` commands to build and install Sphinx
  on your system.

  Note: On some OS X installs you have to adjust the $PATH environment variable
  to make sure that the mysql_config binary was in available (edit .profile and
  add `/opt/local/lib/mysql5/bin`).

  Some helpful links:
  - [Installing Sphinx on OS X Leopard][2]
  - [Install Sphinx Search on Ubuntu Intrepid Ibex][3]

- Searchlight has been tested against [Apache Solr 1.4][4]. If you uncompress
  the download into `/usr/local/apache-solr-1.4.0/` Searchlight will find it
  as expected. You can also specify an alternative path to your Solr install
  using the `--sl-jar-path=[path]` option when using the Searchlight drush
  commands.


3. Install Searchlight on your Drupal site and configure its backend
--------------------------------------------------------------------
Searchlight can be installed like any other Drupal module. Ensure you have all
of its dependencies installed and then enable Searchlight on
`admin/build/modules`.

Once Searchlight is installed you can select the backend you would like to use
on `admin/settings/search/backend`. Any additional settings for your backend
(ports, ttl, etc.) can be configured here.


4. Configure the datasource(s) for your website
-----------------------------------------------
Searchlight allows you to define what content on your site will be indexed.
Searchlight provides a default and very basic `node` table datasource called
`searchlight_node`. If you would like to index content beyond basic node
properties you should edit the existing datasource or create a new one at
`admin/settings/search/datasource`. You can also create new datasources for
indexing other base tables like users or comments.


5. Generate configuration files for your search service
-------------------------------------------------------
Once you have configured and chosen the datasources you would like to use, you
will need to generate configuration files for your Search backend service that
correspond to your Drupal configuration. To do this run the following drush
command for your site:

    $ drush searchlight-conf

The command will write configuration files for your search backend. You can
(and should) re-run this command whenever your datasource configurations change
so that your search service accurately reflects your Drupal configuration.


6. Index your content and start your search backend service
-----------------------------------------------------------
Once your configuration files are written, you will need to index the content
on your website.

For Sphinx, you must run the indexer first before starting the search service:

    $ drush searchlight-index
    $ drush searchlight-searchd

For Solr, you must start the search service before indexing:

    $ drush searchlight-searchd
    $ drush searchlight-index

You will probably want to set up a periodic cron job to schedule the indexing
of your content regularly. To do this add the `drush searchlight-index` command
to your crontab.


7. Search for content
---------------------
Searchlight provides a default view at `search/` with an exposed filter that
you can use to test that your search is working. You can also build your own
Views using the Searchlight filter/argument.


[1]: http://www.sphinxsearch.com/downloads.html
[2]: http://www.viget.com/extend/installing-sphinx-on-os-x-leopard
[3]: http://www.hackido.com/2009/01/install-sphinx-search-on-ubuntu.html
[4]: http://www.apache.org/dyn/closer.cgi/lucene/solr/
