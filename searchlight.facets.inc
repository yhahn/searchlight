<?php

class SearchlightFacets {
  var $active;

  function __construct() {
    $this->active = array();
  }

  function setFacet($key, $value) {
    $this->active[$key] = $value;
  }

  function getFacet($key) {
    return isset($this->active[$key]) ? $this->active[$key] : NULL;
  }

  function delFacet($key) {
    if (isset($this->active[$key])) {
      unset($this->active[$key]);
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Get the PURL modifier value for the current set of active facets, adjusted
   * using one of the $op operations:
   *
   * 'add':    Add a facet value for the given key/value pair.
   * 'remove': Add a facet value for the given key/value pair.
   * 'reset':  Drop all other active facets.
   */
  function getModifier($op = 'add', $key, $value = NULL) {
    $modifier = array();
    switch ($op) {
      case 'add':
        $modifier = $this->active;
        $modifier[$key] = $value;
        break;
      case 'remove':
        $modifier = $this->active;
        if (isset($modifier[$key])) {
          unset($modifier[$key]);
        }
        break;
      case 'reset':
        $modifier = array($key => $value);
        break;
    }
    return $this->pack($modifier);
  }

  function pack($value) {
    $keyvals = array();
    foreach ($value as $k => $v) {
      $keyvals[] = "{$k}-{$v}";
    }
    return implode(',', $keyvals);
  }

  function unpack($value) {
    $parsed = array();
    $split = explode(',', $value);
    foreach ($split as $chunk) {
      $keyval = explode('-', $chunk, 2);
      if (count($keyval) === 2) {
        $parsed[$keyval[0]] = $keyval[1];
      }
    }
    return $parsed;
  }
}
