<?php

/**
 * Facet plugin class for timestamps.
 */
class SearchlightFacetDatatypeTimestamp extends SearchlightFacet {
  /**
   * Override of query().
   */
  function query(&$query) {
    if (isset($this->value)) {
      $range = $query->backend->dateRange($this->value, $this->options['granularity']);

      $query->search_filter[] = array(
        'field' => $this->name,
        'operator' => '>',
        'args' => array($range['from']),
      );
      $query->search_filter[] = array(
        'field' => $this->name,
        'operator' => '<',
        'args' => array($range['to']),
      );
    }

    // Add this facet to be built by the backend.
    $limit = isset($this->options['items']) ? $this->options['items'] : 5;
    $query->add_search_facet($this->name, $limit, array('granularity' => $this->options['granularity']));
  }

  /**
   * Render all items of a facet.
   */
  function render($query, $delta) {
    // We init the view solely for the generation of labels via the field handler.
    $this->viewInit($query);

    switch ($delta) {
      case 'active':
        if (isset($this->value)) {
          $items = array($this->value => array('id' => $this->value));
        }
        break;
      case 'facets':
        if (!isset($this->value)) {
          $items = $query->get_search_facet($this->name);
        }
        break;
    }
    if (!empty($items)) {
      foreach ($items as $k => $item) {
        $format = $this->options['date_format'];
        $timestamp = is_numeric($item['id']) ? $item['id'] : strtotime($item['id']);
        $items[$k]['title'] = format_date($timestamp, 'custom', $format, 0);
      }
      return $items;
    }
    return array();
  }

  /**
   * Provide default values for options.
   */
  function optionsDefault() {
    $options = parent::optionsDefault();
    $options['granularity'] = 'month';
    $options['date_format'] = 'F, Y';
    return $options;
  }

  /**
   * Provide an options form to be exposed in the Environment editor.
   */
  function extendedForm(&$form, $form_state) {
    parent::extendedForm($form, $form_state);
    $form['granularity'] = array(
      '#title' => t('Granularity'),
      '#type' => 'select',
      '#options' => array(
        'year' => t('By year'),
        'month' => t('By month'),
        'day' => t('By day'),
      ),
      '#default_value' => $this->options['granularity'],
    );
    $form['date_format'] = array(
      '#title' => t('Date format'),
      '#type' => 'textfield',
      '#size' => 10,
      '#default_value' => $this->options['date_format'],
    );
  }
}
