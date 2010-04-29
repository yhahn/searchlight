<?php

class SearchlightFacets {
  var $active;

  function __construct() {
    $this->active = array();
  }

  /**
   * Retrieve any valid active facets for the provided datasource.
   */
  function allFacets($datasource) {
    $facets = array();
    foreach ($datasource->fields as $name => $field) {
      // if (!empty($field['facet'])) {
        $facets[$name] = $field;
      // }
    }
    return $facets;
  }

  /**
   * Retrieve any valid active facets for the provided datasource.
   */
  function inactiveFacets($datasource) {
    $facets = $this->allFacets($datasource);
    return array_diff_key($facets, $this->active);
  }

  /**
   * Retrieve any valid active facets for the provided datasource.
   */
  function activeFacets($datasource) {
    $facets = $this->allFacets($datasource);
    return array_intersect_key($facets, $this->active);
  }

  /**
   * Set a facet value to be active.
   */
  function setFacet($key, $value) {
    $this->active[$key] = $value;
  }

  /**
   * Get a given active facet's value.
   */
  function getFacet($key = NULL) {
    if (isset($key)) {
      return isset($this->active[$key]) ? $this->active[$key] : NULL;
    }
    return $this->active;
  }

  /**
   * Remove a facet from the active facets array.
   */
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

  /**
   * Pack a set of active facets into a usable PURL modifier string.
   */
  function pack($value) {
    $keyvals = array();
    foreach ($value as $k => $v) {
      $keyvals[] = "{$k}-{$v}";
    }
    return implode(',', $keyvals);
  }

  /**
   * Unpack a set of active facets from a PURL modifier string.
   */
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

  /**
   * Render a facet using its corresponding field handler.
   */
  function render($datasource, $field, $items) {
    if ($field['usage'] === 'multivalue') {
      $view = searchlight_build_view($field['view']);
    }
    else {
      $view = $datasource->view->copy();
      $view->build();
    }

    // Detect which handler is responsible for this field and build fake
    // resultset for it to process.
    if ($field['usage'] === 'multivalue') {
      foreach ($view->field as $field_handler) {
        if (in_array($field_handler->field_alias, array($field['name'], $field['label_field']['name']), TRUE)) {
          $handler = $field_handler;

          // Multivalue fields must build a new Views query in order to
          // retrieve any secondary label field values.
          $query = views_get_plugin('query', 'views_query');
          $query->init($field['table'], $field['field']);

          $ids = array_keys($items);

          // Add WHERE condition on item ids.
          $query->add_where(0, "{$field['table']}.{$field['field']} IN (". db_placeholders($ids, 'int') .")", $ids);

          // Add base field.
          $field_alias = $query->add_field($field['table'], $field['field']);

          // Add label field.
          $label_alias = $query->add_field($field['label_field']['table'], $field['label_field']['field']);
          $result = db_query($query->query(), $ids);

          $rows = array();
          while ($row = db_fetch_object($result)) {
            $row->{$field['name']} = $row->{$field['field']};
            $row->{$field['label_field']['name']} = $row->{$label_alias};
            $rows[$row->{$field['name']}] = $row;
          }
          break;
        }
      }
    }
    else {
      foreach ($view->field as $field_handler) {
        if ($field_handler->field_alias === $field['name']) {
          $handler = $field_handler;
          $rows = array();
          foreach ($items as $item) {
            $row = new stdClass();
            $row->{$field['name']} = $item['id'];
            $rows[$item['id']] = $row;
          }
          break;
        }
      }
    }
    // Fake some rows so we can use the pre_render method.
    if (!empty($handler)) {
      $handler->pre_render($rows);

      // Now render each row.
      $facets = searchlight_facets();
      $rendered = array();

      // Active facet.
      if ($facets->getFacet($field['name'])) {
        foreach ($rows as $id => $row) {
          // Generate a PURL modifier for this item.
          $modifier = $facets->getModifier('remove', $field['name'], $id);
          $item = $items[$id];
          $item['title'] = strip_tags($handler->render($row));
          if (!empty($modifier)) {
            $item['link'] = l(t('remove'), $_GET['q'], array(
              'purl' => array('provider' => 'searchlight', 'id' => $modifier))
            );
          }
          else {
            $item['link'] = l(t('remove'), $_GET['q'], array(
              'purl' => array('remove' => array('searchlight')))
            );
          }
          $rendered[$id] = theme('searchlight_facet_active', $field, $item);
        }
      }
      // Inactive facet.
      else {
        foreach ($rows as $id => $row) {
          // Generate a PURL modifier for this item.
          $modifier = $facets->getModifier('add', $field['name'], $id);
          $item = $items[$id];
          $item['title'] = strip_tags($handler->render($row));
          $item['link'] = l($item['title'], $_GET['q'], array(
            'query' => drupal_query_string_encode($_GET, array('q')),
            'purl' => array('provider' => 'searchlight', 'id' => $modifier))
          );
          $rendered[$id] = theme('searchlight_facet_link', $field, $item);
        }
      }
      return array('label' => $handler->label(), 'items' => $rendered);
    }
    return FALSE;
  }
}
