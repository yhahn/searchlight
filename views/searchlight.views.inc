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
    ),
  );
}

/**
 * Implementation of hook_views_data().
 */
function searchlight_views_data() {
  $data = array();
  $data['searchlight']['table']['group'] = t('Global');
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
 * Implementation of hook_views_query_alter().
 */
function searchlight_views_query_alter(&$view, &$query) {
  if (isset($view->searchlight) && $backend = searchlight_default_backend()) {
    $backend->viewsExecute($view);
  }
}
