<?php

class SearchlightEnvironment {
  var $name;
  var $datasource;
  var $facets;
  var $options;

  function __construct() {
    $this->name = '';
    $this->datasource = '';
    $this->facets = array();
    $this->options = array();
  }
}
