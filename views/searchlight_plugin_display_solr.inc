<?php

class searchlight_plugin_display_solr extends views_plugin_display {
  function render() {
    $return = array();
    foreach  ($this->view->result as $row) {
      $render = array();
      foreach ($this->view->field as $id => $field) {
        $render[$id] = $field->advanced_render($row);
      }
      $return[] = $render;
    }
    return $return;
  }
}
