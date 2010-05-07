<?php

/**
 * Implementation of hook_searchlight_default_datasources().
 */
function searchlight_searchlight_default_datasources() {
  module_load_include('inc', 'searchlight', 'searchlight.defaults');
  $args = func_get_args();
  return call_user_func_array('_searchlight_searchlight_default_datasources', $args);
}

/**
 * Implementation of hook_searchlight_default_environments().
 */
function searchlight_searchlight_default_environments() {
  module_load_include('inc', 'searchlight', 'searchlight.defaults');
  $args = func_get_args();
  return call_user_func_array('_searchlight_searchlight_default_environments', $args);
}

/**
 * Implementation of hook_views_default_views().
 */
function searchlight_views_default_views() {
  module_load_include('inc', 'searchlight', 'searchlight.features.views');
  $args = func_get_args();
  return call_user_func_array('_searchlight_views_default_views', $args);
}
