<?php
// $Id$

/**
 * Implementation of hook_views_plugins().
 */
function searchlight_views_plugins() {
  $base = array(
    'path' => drupal_get_path('module', 'searchlight') . '/views',
    'theme' => 'views_view',
    'uses hook menu' => FALSE,
    'use ajax' => FALSE,
    'use pager' => FALSE,
    'accept attachments' => FALSE,
  );
  return array(
    'query' => array(
      'searchlight_sphinx' => array(
        'title' => t('Searchlight Sphinx'),
        'help' => t('Searchlight Sphinx query plugin.'),
        'handler' => 'searchlight_plugin_query_sphinx',
        'parent' => 'views_query',
      ),
    ),
    'display' => array(
      'searchlight_datasource' => $base + array(
        'title' => t('Searchlight datasource'),
        'help' => t('Expose a base table to Searchlight for indexing.'),
        'handler' => 'searchlight_plugin_display_datasource',
        'admin' => t('Searchlight datasource'),
      ),
      'searchlight_multivalue' => $base + array(
        'title' => t('Searchlight multivalue'),
        'help' => t('Expose a view as an multivalue attribute to be used with a Searchlight datasource.'),
        'handler' => 'searchlight_plugin_display_multivalue',
        'admin' => t('Searchlight multivalue'),
        'parent' => 'searchlight_datasource',
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
 * Implementation of hook_views_query_alter().
 */
function searchlight_views_query_alter(&$view, &$query) {
  if (isset($view->searchlight) && $backend = searchlight_default_backend()) {
    $backend->viewsExecute($view);
  }
}

/**
 * Implementation of hook_views_default_views_alter().
 */
function searchlight_views_default_views_alter(&$views) {
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
    $view->base_table = 'node';
    $view->api_version = 2;
    foreach ($realms as $realm) {
      $handler = $view->new_display('searchlight_multivalue', $realm, "searchlight_{$realm}");
      $handler->override_option('arguments', array());
      $handler->override_option('filters', array());
      $handler->override_option('relationships', array());
      $handler->override_option('sorts', array());
      $handler->override_option('fields', array(
        'realm' => array(
          'id' => 'nid',
          'table' => 'node_access',
          'field' => 'nid',
          'relationship' => 'none',
          'realm' => $realm,
        ),
        'nid' => array(
          'id' => 'nid',
          'table' => 'node',
          'field' => 'nid',
          'relationship' => 'none',
        ),
      ));
      $handler->override_option('searchlight_multivalue', array(
        'field' => "node_access_gid_{$realm}",
        'label_field' => "node_access_gid_{$realm}",
      ));
    }
    $views[$view->name] = $view;
  }
}
