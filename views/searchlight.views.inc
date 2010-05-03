<?php
// $Id$

/**
 * Implementation of hook_views_plugins().
 */
function searchlight_views_plugins() {
  return array(
    'query' => array(
      'searchlight' => array(
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
 * Implementation of hook_views_handlers().
 */
function searchlight_views_handlers() {
  return array(
    'info' => array('path' => drupal_get_path('module', 'searchlight') .'/views'),
    'handlers' => array(
      'searchlight_handler_argument_search' => array('parent' => 'views_handler_argument'),
      'searchlight_handler_filter_search' => array('parent' => 'views_handler_filter'),
      'searchlight_handler_sort_search' => array('parent' => 'views_handler_sort'),
      'searchlight_handler_field_node_access' => array('parent' => 'views_handler_field'),
    ),
  );
}

/**
 * Implementation of hook_views_data().
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
  return $data;
}

/**
 * Implementation of hook_views_data_alter().
 */
function searchlight_views_data_alter(&$data) {
  if (!empty($data['node_access']['nid']) && empty($data['node_access']['nid']['field'])) {
    $data['node_access']['nid']['field'] = array(
      'handler' => 'searchlight_handler_field_node_access',
      'help' => t('Node access fields for indexing by Searchlight search backends.'),
    );
  }
}

/**
 * Implementation of hook_views_pre_build().
 */
function searchlight_views_pre_build(&$view) {
  $searchlight = FALSE;

  // Init the searchlight query plugin if the view uses a searchlight filter or argument.
  foreach (array('filter', 'argument') as $type) {
    $handlers = $view->display_handler->get_handlers($type);
    foreach ($handlers as $handler) {
      if ($handler->table === 'searchlight') {
        $searchlight = TRUE;
        break;
      }
    }
  }
  // The following is a copy of the $view->query_init() method, with the only
  // difference being that we use the searchlight backend plugin rather than
  // the default views query plugin and pass the backend to the query plugin.
  if ($searchlight && $backend = searchlight_get_backend()) {
    if (!empty($view->query)) {
      $class = get_class($view->query);
      if ($class && $class != 'stdClass') {
        // return if query is already initialized.
        return;
      }
    }

    // Create and initialize the query object.
    $views_data = views_fetch_data($view->base_table);
    $view->base_field = $views_data['table']['base']['field'];
    if (!empty($views_data['table']['base']['database'])) {
      $view->base_database = $views_data['table']['base']['database'];
    }

    // Create and initialize the query object.
    $view->query = views_get_plugin('query', 'searchlight');
    $view->query->init($view->base_table, $view->base_field, $backend);
  }
}

/**
 * Implementation of hook_views_query_alter().
 */
function searchlight_views_query_alter(&$view) {
  if (!empty($view->query->searchlight)) {
    // Retrieve active facets. We will consider the presence of any active
    // facets to mean the search query is *not* empty.
    $active_facets = searchlight_facet_manager()->activeFacets($view->query->datasource);

    if (!empty($active_facets)) {
      $view->query->search_options['hideEmpty'] = FALSE;

      // Set filters for active facets. Note that query->add_where() is **NOT**
      // used as this may force undesirable joins on the SQL version of the
      // query.
      foreach ($active_facets as $name => $field) {
        $arg = searchlight_facet_manager()->getFacet($name);
        switch ($field['datatype']) {
          case 'timestamp':
            $granularity = !empty($field['granularity']) ? $field['granularity'] : 'month';
            $range = $view->query->backend->dateRange($arg, $granularity);

            $view->query->search_filter[] = array(
              'field' => $field['name'],
              'operator' => '>',
              'args' => array($range['from']),
            );
            $view->query->search_filter[] = array(
              'field' => $field['name'],
              'operator' => '<',
              'args' => array($range['to']),
            );
            break;
          default:
            $view->query->search_filter[] = array(
              'field' => $field['name'],
              'operator' => '=',
              'args' => array($arg),
            );
            break;
        }
      }
    }
  }
}

/**
 * Implementation of hook_views_default_views_alter().
 */
function searchlight_views_default_views_alter(&$views) {
  //
  // Node access ==============================================================
  //
  $realms = array();
  $result = db_query("SELECT realm FROM {node_access} GROUP BY realm");
  while ($row = db_fetch_object($result)) {
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
    $view->api_version = 3.0-alpha1;
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
    $view->api_version = 3.0-alpha1;
    $handler = $view->new_display('default', 'Default', 'default');
    foreach (taxonomy_get_vocabularies() as $vocab) {
      /* Display: Vocabulary */
      $handler = $view->new_display('searchlight_multivalue', $vocab->name, 'searchlight_multivalue_'. $vocab->vid);
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
        'name' => array(
          'id' => 'name',
          'table' => 'term_data',
          'field' => 'name',
          'relationship' => 'none',
          'label' => $vocab->name,
        ),
      ));
      $handler->override_option('filters', array(
        'vid' => array(
          'id' => 'vid',
          'table' => 'term_data',
          'field' => 'vid',
          'value' => array($vocab->vid => $vocab->vid),
        ),
      ));
      $handler->override_option('searchlight_multivalue', array(
        'field' => 'term_data_tid',
        'label_field' => 'term_data_name',
        'override' => array(
          'name' => 'term_data_tid_v'. $vocab->vid,
          'label' => $vocab->name,
        ),
      ));
    }
    $views[$view->name] = $view;
  }
}
