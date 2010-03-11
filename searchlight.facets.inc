<?php

class SearchlightFacets {
  var $active;

  function __construct() {
    $this->active = array();
  }

  /**
   * Retrieve any valid active facets for the provided datasource.
   */
  function activeFacets($datasource) {
    $facets = array();
    foreach ($this->active as $key => $value) {
      if (isset($datasource['fields'][$key])) {
        $facets[$key] = $value;
      }
    }
    return $facets;
  }

  function setFacet($key, $value) {
    $this->active[$key] = $value;
  }

  function getFacet($key = NULL) {
    if (isset($key)) {
      return isset($this->active[$key]) ? $this->active[$key] : NULL;
    }
    return $this->active;
  }

  function delFacet($key = NULL) {
    if (isset($key)) {
      if (isset($this->active[$key])) {
        unset($this->active[$key]);
        return TRUE;
      }
      return FALSE;
    }
    $this->active = array();
    return TRUE;
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
