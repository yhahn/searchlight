<?php
// $Id$


/**
 * Implements hook_views_plugins().
 */
function searchlight_views_plugins() {
  return array(
    'query' => array(
      'searchlight' => array(
        'file' => 'searchlight_plugin_query_v3.inc',
        'path' => drupal_get_path('module', 'searchlight') . '/views',
        'title' => t('Searchlight'),
        'help' => t('Searchlight query plugin.'),
        'handler' => 'searchlight_plugin_query',
        'parent' => 'views_query',
      ),
    ),
    'display' => array(
      'searchlight_multivalue' => array(
        'path' => drupal_get_path('module', 'searchlight') . '/views',
        'theme' => 'views_view',
        'uses hook menu' => FALSE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
        'title' => t('Searchlight multivalue'),
        'help' => t('Expose a view as an multivalue attribute to be used with a Searchlight datasource.'),
        'handler' => 'searchlight_plugin_display_multivalue',
        'admin' => t('Searchlight multivalue'),
      ),
      'searchlight_solr' => array(
        'path' => drupal_get_path('module', 'searchlight') . '/views',
        'theme' => 'views_view',
        'uses hook menu' => FALSE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
        'title' => t('Searchlight Solr'),
        'help' => t('Expose a view as an multivalue attribute to be used with a Searchlight datasource.'),
        'handler' => 'searchlight_plugin_display_solr',
        'admin' => t('Searchlight Solr'),
      ),
    ),
  );
}

/**
 * Implements hook_views_handlers().
 */
function searchlight_views_handlers() {
  return array(
    'info' => array('path' => drupal_get_path('module', 'searchlight') . '/views'),
    'handlers' => array(
      'searchlight_handler_argument_search' => array('parent' => 'views_handler_argument'),
      'searchlight_handler_filter_search' => array('parent' => 'views_handler_filter'),
      'searchlight_handler_filter_facets' => array('parent' => 'views_handler_filter'),
      'searchlight_handler_sort_search' => array('parent' => 'views_handler_sort'),
      'searchlight_handler_field_node_access' => array('parent' => 'views_handler_field'),
      'searchlight_handler_field_facet_link' => array('parent' => 'views_handler_field'),
    ),
  );
}

/**
 * Implements hook_views_data().
 */
function searchlight_views_data() {
  $data = array();
  $data['searchlight']['table']['group'] = t('Search');
  $data['searchlight']['table']['join'] = array(
    '#global' => array(),
  );
  $data['searchlight']['search'] = array(
    'title' => t('Searchlight'),
    'help' => t('Filter results by a Searchlight search.'),
    'argument' => array('handler' => 'searchlight_handler_argument_search'),
    'filter' => array('handler' => 'searchlight_handler_filter_search'),
    'sort' => array('handler' => 'searchlight_handler_sort_search'),
  );
  $data['searchlight']['facets'] = array(
    'title' => t('Active facets'),
    'help' => t('Filter results by active Searchlight facets.'),
    'filter' => array('handler' => 'searchlight_handler_filter_facets'),
  );
  $data['searchlight']['facet_link'] = array(
    'title' => t('Facet URL'),
    'help' => t('Generate a facet URL that can be used as a Views render token.'),
    'field' => array('handler' => 'searchlight_handler_field_facet_link'),
  );
  return $data;
}

/**
 * Implements hook_views_data_alter().
 */
function searchlight_views_data_alter(&$data) {
  if (!empty($data['node_access']['nid']) && empty($data['node_access']['nid']['field'])) {
    $data['node_access']['nid']['field'] = array(
      'handler' => 'searchlight_handler_field_node_access',
      'help' => t('Node access fields for indexing by Searchlight search backends.'),
    );
  }

  // A special handler for taxonomy_index table. Taxonomy views integration
  // in D7 sometimes makes use of `taxonomy_term_data` but when used for
  // arguments, filters, etc. directly queries against `taxonomy_index`. 
  if (module_exists('taxonomy')) {
    $data['taxonomy_index']['searchlight_tid'] = array(
      'title' => t('Term ID (Searchlight)'),
      'help' => t('The taxonomy term ID'),
      'real field' => 'tid',
      'field' => array(
        'field' => 'tid',
        'handler' => 'views_handler_field_numeric',
        'click sortable' => TRUE,
      ),
    );
  }

  // OG UID field handler. Needed to allow filtering directly against OG UID.
  if (module_exists('og_views')) {
    $data['og_uid']['nid']['field'] = array(
      'title' => t('Group ID'),
      'handler' => 'views_handler_field',
    );
  }

  // If Searchlight buddy system is off, replace core search handlers.
  if (module_exists('search') && !variable_get('searchlight_buddysystem', FALSE)) {
    $data['search_index']['score']['sort'] = $data['searchlight']['search']['sort'];
    $data['search_index']['keys']['filter'] = $data['searchlight']['search']['filter'];

    // Remove unsupported tables.
    unset($data['search_index']['score']['field']);
    unset($data['search_dataset']);
    unset($data['search_total']);
    unset($data['search_node_links_from']);
    unset($data['search_node_links_to']);
  }
}

/**
 * Init (or more accurately "hijack") the Views query plugin. This is called
 * from the pre_query() method of the searchlight filter, argument handlers.
 */
function searchlight_views_init_query(&$view) {
  // The following is a copy of the $view->query_init() method, with the only
  // difference being that we use the searchlight backend plugin rather than
  // the default views query plugin and pass the backend to the query plugin.
  if (!empty($view->query)) {
    $class = get_class($view->query);
    if ($class && $class != 'stdClass' && !empty($view->query->searchlight)) {
      // return if query is already initialized.
      return;
    }
  }

  // Create and initialize the query object. If no backend is present, leave
  // the view alone.
  if ($backend = searchlight_get_backend()) {
    $views_data = views_fetch_data($view->base_table);
    $view->base_field = $views_data['table']['base']['field'];
    if (!empty($views_data['table']['base']['database'])) {
      $view->base_database = $views_data['table']['base']['database'];
    }

    // Check that an active datasource can be found for this base table.
    if (searchlight_get_datasource($view->base_table)) {
      // Add Searchlight JS.
      drupal_add_js(drupal_get_path('module', 'searchlight') . '/searchlight.js');

      $query_options = $view->display_handler->get_option('query');
      $view->query = searchlight_get_query($view->base_table, $view->base_field, $query_options['options']);
    }
  }
}

/**
 * Implements hook_views_default_views().
 */
function searchlight_views_default_views() {
  $views = array();

  //
  // Node access ==============================================================
  //
  $realms = array();
  $result = db_query("SELECT realm FROM {node_access} GROUP BY realm");
  foreach ($result as $row) {
    $realms[$row->realm] = $row->realm;
  }
  // Allow modules to declare their realms in an alter hook. This allows
  // searchlight datasources to include references to node_access realms
  // prior to the creation of any nodes that actually use those realms in
  // node_access entries.
  drupal_alter('searchlight_node_access_realms', $realms);

  if (!empty($realms)) {
    $view = new view;
    $view->name = 'searchlight_node_access';
    $view->tag = 'searchlight';
    $view->base_table = 'node';
    $view->api_version = '3.0-alpha1';
    $handler = $view->new_display('default', 'Default', 'default');
    foreach ($realms as $realm) {
      $handler = $view->new_display('searchlight_multivalue', $realm, "searchlight_{$realm}");
      $handler->override_option('arguments', array());
      $handler->override_option('filters', array());
      $handler->override_option('relationships', array());
      $handler->override_option('sorts', array());
      $handler->override_option('fields', array(
        'nid' => array(
          'id' => 'nid',
          'table' => 'node',
          'field' => 'nid',
          'relationship' => 'none',
        ),
        'realm' => array(
          'id' => 'nid',
          'table' => 'node_access',
          'field' => 'nid',
          'relationship' => 'none',
          'realm' => $realm,
        ),
      ));
      $handler->override_option('searchlight_multivalue', array(
        'field' => "node_access_gid_{$realm}",
        'label_field' => "node_access_gid_{$realm}",
      ));
    }
    $views[$view->name] = $view;
  }

  //
  // Taxonomy =================================================================
  //
  if (module_exists('taxonomy')) {
    $view = new view;
    $view->name = 'searchlight_term_data';
    $view->tag = 'searchlight';
    $view->base_table = 'node';
    $view->api_version = '3.0-alpha1';
    $handler = $view->new_display('default', 'Default', 'default');
    /* Display: All terms */
    $handler = $view->new_display('searchlight_multivalue', 'all', 'searchlight_multivalue_all');
    $handler->override_option('arguments', array());
    $handler->override_option('relationships', array());
    $handler->override_option('sorts', array());
    $handler->override_option('fields', array(
      'nid' => array(
        'id' => 'nid',
        'table' => 'node',
        'field' => 'nid',
        'relationship' => 'none',
      ),
      'seachlight_tid' => array(
        'id' => 'searchlight_tid',
        'table' => 'taxonomy_index',
        'field' => 'searchlight_tid',
        'relationship' => 'none',
        'label' => t('Term ID'),
      ),
      'name' => array(
        'id' => 'name',
        'table' => 'taxonomy_term_data',
        'field' => 'name',
        'relationship' => 'none',
        'label' => t('All terms'),
      ),
    ));
    $handler->override_option('searchlight_multivalue', array(
      'field' => 'taxonomy_index_tid',
      'label_field' => 'taxonomy_term_data_name',
      'override' => array(
        'name' => 'taxonomy_index_tid',
        'label' => t('Taxonomy (all)'),
      ),
    ));
    foreach (taxonomy_get_vocabularies() as $vocab) {
      /* Display: Vocabulary */
      $handler = $view->new_display('searchlight_multivalue', $vocab->name, 'searchlight_multivalue_' . $vocab->machine_name);
      $handler->override_option('arguments', array());
      $handler->override_option('relationships', array());
      $handler->override_option('sorts', array());
      $handler->override_option('fields', array(
        'nid' => array(
          'id' => 'nid',
          'table' => 'node',
          'field' => 'nid',
          'relationship' => 'none',
        ),
        'seachlight_tid' => array(
          'id' => 'searchlight_tid',
          'table' => 'taxonomy_index',
          'field' => 'searchlight_tid',
          'relationship' => 'none',
          'label' => t('Term ID'),
        ),
        'name' => array(
          'id' => 'name',
          'table' => 'taxonomy_term_data',
          'field' => 'name',
          'relationship' => 'none',
          'label' => $vocab->name,
        ),
      ));
      $handler->override_option('filters', array(
        'machine_name' => array(
          'id' => 'machine_name',
          'table' => 'taxonomy_vocabulary',
          'field' => 'machine_name',
          'value' => array($vocab->machine_name => $vocab->machine_name),
        ),
      ));
      $handler->override_option('searchlight_multivalue', array(
        'field' => 'taxonomy_index_tid',
        'label_field' => 'taxonomy_term_data_name',
        'override' => array(
          'name' => 'taxonomy_index_tid_' . $vocab->machine_name,
          'label' => $vocab->name,
        ),
      ));
    }
    $views[$view->name] = $view;
  }

  return $views;
}
