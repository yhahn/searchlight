<?php

/**
 * Implements hook_ctools_plugin_api().
 */
function searchlight_basic_ctools_plugin_api() {
  module_load_include('inc', 'searchlight_basic', 'searchlight_basic.defaults');
  $args = func_get_args();
  return call_user_func_array('_searchlight_basic_ctools_plugin_api', $args);
}

/**
 * Implements hook_searchlight_default_datasources().
 */
function searchlight_basic_searchlight_default_datasources() {
  module_load_include('inc', 'searchlight_basic', 'searchlight_basic.defaults');
  $args = func_get_args();
  return call_user_func_array('_searchlight_basic_searchlight_default_datasources', $args);
}

/**
 * Implements hook_searchlight_default_environments().
 */
function searchlight_basic_searchlight_default_environments() {
  module_load_include('inc', 'searchlight_basic', 'searchlight_basic.defaults');
  $args = func_get_args();
  return call_user_func_array('_searchlight_basic_searchlight_default_environments', $args);
}

/**
 * Implements hook_views_default_views().
 */
function searchlight_basic_views_default_views() {
  module_load_include('inc', 'searchlight_basic', 'searchlight_basic.features.views');
  $args = func_get_args();
  return call_user_func_array('_searchlight_basic_views_default_views', $args);
}
