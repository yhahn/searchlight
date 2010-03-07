<?php

class searchlight_backend {
  var $settings;

  function settingsForm($values) {
    return array();
  }

  function settingsDefault() {
    return array();
  }

  function __construct($id) {
    $this->id = $id;
    $this->settings = variable_get("searchlight_backend_{$this->id}", array()) + $this->settingsDefault();
  }
}
