<?php
// $Id$

/**
 * Implementation of hook_views_plugins().
 */
function searchlight_views_plugins() {
  return array(
    // 'module' => 'sphinx_views',
    'display' => array(
      'searchlight_datasource' => array(
        'title' => t('Searchlight datasource'),
        'help' => t('Expose a base table to Searchlight for indexing.'),
        'handler' => 'searchlight_plugin_display_datasource',
        'path' => drupal_get_path('module', 'searchlight') . '/views',
        'theme' => 'views_view',
        'uses hook menu' => FALSE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
        'admin' => t('Searchlight datasource'),
      ),
      'searchlight_attribute' => array(
        'title' => t('Searchlight attribute'),
        'help' => t('Expose a view as an attribute to be used with a Searchlight datasource.'),
        'handler' => 'searchlight_plugin_display_attribute',
        'path' => drupal_get_path('module', 'searchlight') . '/views',
        'theme' => 'views_view',
        'uses hook menu' => FALSE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
        'admin' => t('Searchlight attribute'),
        'parent' => 'searchlight_datasource',
      ),
    ),
    /*
    'argument default' => array(
      'node_title' => array(
        'title' => t('Node title from URL'),
        'handler' => 'sphinx_views_plugin_argument_default_node_title',
        'file' => 'sphinx_views_plugin_argument_default_node_title.inc',
        'path' => drupal_get_path('module', 'sphinx_views') . '/views',
        'parent' => 'fixed',
      ),
    ),
    */
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
  return;
  // @TODO: Backend plugin interface point.
  if (isset($view->searchlight)) {
    module_load_include('inc', 'searchlight', 'sphinx_views');
    $sphinx_view = new sphinx_view($view);
    $sphinx_view->execute_view();

    // If this is a page view, set it in the static cache.
    if ($view->display_handler->display->display_plugin === 'page') {
      sphinx_views_current_sphinx_view($sphinx_view);
    }
  }
}
