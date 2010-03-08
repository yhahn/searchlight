<?php

/**
 * Implementation of hook_views_default_views().
 */
function searchlight_views_default_views() {
  module_load_include('inc', 'searchlight', 'searchlight.features.views');
  $args = func_get_args();
  return call_user_func_array('_searchlight_views_default_views', $args);
}
