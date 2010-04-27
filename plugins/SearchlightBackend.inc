<?php

/**
 * Abstract class, defines interface for swappable search backends.
 */
class SearchlightBackend {
  var $settings;
  var $views;

  /**
   * Constructor. Unpack backend variable & populate default settings.
   */
  function __construct($id) {
    $this->id = $id;
    $this->settings = variable_get("searchlight_backend_{$this->id}", array()) + $this->settingsDefault();
  }

  /**
   * Array of default settings for this search backend.
   */
  function settingsDefault() {
    return array();
  }

  /**
   * System settings form for this search backend.
   */
  function settingsForm($values) {
    return array();
  }

  /**
   * Views integration methods ================================================
   */

  /**
   * Backend callback for filter, argument views handler
   * $handler->option_definition() method.
   */
  function viewsOptionDefinition(&$handler) {
    return array(
      'hideEmpty' => array('default' => TRUE),
    );
  }

  /**
   * Backend callback for filter, argument views handler
   * $handler->options_form() method.
   */
  function viewsOptionsForm(&$form, &$form_state, &$handler) {
    $form['hideEmpty'] = array(
      '#title' => t('Hide all results when empty'),
      '#type' => 'checkbox',
      '#default_value' => $handler->options['hideEmpty'],
    );
  }

  /**
   * Backend callback for filter handler $handler->value_form() method.
   */
  function viewsValueForm(&$form, &$form_state, &$handler) {
    $form['value'] = array(
      '#type' => 'textfield',
      '#title' => t('Value'),
      '#size' => 30,
      '#default_value' => $this->value,
    );
    if (!empty($form_state['exposed']) && !isset($form_state['input'][$identifier])) {
      $form_state['input'][$identifier] = $this->value;
    }
  }

  /**
   * Initialize a client for the search backend.
   */
  function initClient() {
  }

  /**
   * Execute a query using the search backend.
   */
  function executeQuery(&$client, $datasource, $query = '') {
  }

  /**
   * Set any custom options for the search backend.
   */
  function setOptions(&$client, $options) {
  }

  /**
   * Set a filter parameter for the search backend.
   */
  function setFilter(&$client, $datasource, $filters) {
  }

  /**
   * Set a sort parameter for the search backend.
   */
  function setSort(&$client, $sorts) {
  }

  /**
   * Set a pager/limit parameter for the search backend.
   */
  function setPager(&$client, $offset, $limit) {
  }

  /**
   * Set node_access attribute filters.
   */
  function setNodeAccess(&$client) {
  }

  /**
   * Utility date methods =====================================================
   */

  /**
   * Retrieve a from & to timestamps for a given granularity unit and base
   * timestamp.
   */
  function dateRange($timestamp, $granularity = 'month') {
    switch ($granularity) {
      case 'month':
        $from = mktime(0, 0, 0, date('m', $timestamp), 1, date('Y', $timestamp));
        $to   = mktime(23, 59, 59, date('m', $timestamp), cal_days_in_month(CAL_GREGORIAN, date('m', $timestamp), date('Y', $timestamp)), date('Y', $timestamp));
        break;
      case 'day':
        $from = mktime(0, 0, 0, date('m', $timestamp), date('d', $timestamp), date('Y', $timestamp));
        $to   = mktime(23, 59, 59, date('m', $timestamp), date('d', $timestamp), date('Y', $timestamp));
        break;
      case 'year':
        $from = mktime(0, 0, 0, 1, 1, date('Y', $timestamp));
        $to   = mktime(23, 59, 59, 12, cal_days_in_month(CAL_GREGORIAN, 12, date('Y', $timestamp)), date('Y', $timestamp));
        break;
    }
    return array('from' => $from, 'to' => $to);
  }

  /**
   * Facet methods ============================================================
   */

  /**
   * Execute backend operations to determine the top facets for a given field.
   *
   * @return A result array, each with a sub-array of items containing:
   *
   * 'id': The primary ID for this item.
   * 'count': The number of entries that belong to this facet item.
   */
  function facetBuild(&$searchlight, $field) {
    return array();
  }

  /**
   * Render a facet using its corresponding field handler.
   */
  function facetRender(&$searchlight, $field, $items) {
    $view = ($field['usage'] === 'multivalue') ? $field['view'] : $searchlight['datasource']['view'];
    $view = searchlight_build_view($view);

    // Detect which handler is responsible for this field and build fake
    // resultset for it to process.
    if ($field['usage'] === 'multivalue') {
      foreach ($view->field as $field_handler) {
        if (in_array($field_handler->field_alias, array($field['name'], $field['label_field']['name'], TRUE))) {
          $handler = $field_handler;

          // Multivalue fields must build a new Views query in order to
          // retrieve any secondary label field values.
          $query = new views_query($field['table'], $field['field']);
          $ids = array_keys($items);

          // Add WHERE condition on item ids.
          $query->add_where(0, "{$field['table']}.{$field['field']} IN (". db_placeholders($ids, 'int') .")", $ids);

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
            $item['link'] = l(t('remove'), $_GET['q'], array('purl' => array('provider' => 'searchlight', 'id' => $modifier)));
          }
          else {
            $item['link'] = l(t('remove'), $_GET['q'], array('purl' => array('remove' => array('searchlight'))));
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
          $item['link'] = l($item['title'], $_GET['q'], array('purl' => array('provider' => 'searchlight', 'id' => $modifier)));
          $rendered[$id] = theme('searchlight_facet_link', $field, $item);
        }
      }
      return array('label' => $handler->label(), 'items' => $rendered);
    }
    return FALSE;
  }
}
