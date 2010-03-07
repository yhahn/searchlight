<?php

class searchlight_backend {
  var $settings;

  public function settingsForm($values) {
    return array();
  }

  protected function settingsDefault() {
    return array();
  }

  function __construct($id) {
    $this->id = $id;
    $this->settings = variable_get("searchlight_backend_{$this->id}", array()) + $this->settingsDefault();
  }
}
