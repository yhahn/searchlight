<?php
/**
 * Default argument plugin to extract a node title via menu_get_object.
 */
class sphinx_views_plugin_argument_default_node_title extends views_plugin_argument_default {
  var $option_name = 'default_argument_node_title';

  function argument_form(&$form, &$form_state) {
    $this->check_access($form);
  }

  function get_argument($raw = FALSE) {
    $node = menu_get_object();
    if (!$node && arg(0) == 'node' && is_numeric(arg(1))) {
      $node = node_load(arg(1));
    }
    if ($node) {
      return $node->title;
    }
    return NULL;
  }
}
