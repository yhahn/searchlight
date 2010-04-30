<?php

class searchlight_plugin_display_solr extends views_plugin_display {
  function render() {
    return $this->view->result;
  }
}
