<?php

/**
 * Facet plugin class that provides a facet corresponding to the current
 * fulltext search query.
 */
class SearchlightFacetSearchQuery extends SearchlightFacet {
  /**
   * Override of construct().
   */
  function construct($environment, $field, $value, $options) {
    $this->name = 'search_query';
    $this->field = $field;
    $this->value = $value;
    $this->options = $options + $this->optionsDefault();
    $this->environment = $environment;

    $this->view = NULL;
    $this->handler = NULL;
  }

  /**
   * Override of query().
   */
  function query(&$query) { }

  /**
   * Generate a label for this facet.
   */
  function label($delta) {
    if (!empty($this->options['label'])) {
      return $this->options['label'];
    }
    return t('Search');
  }

  /**
   * Override of theme().
   * Only theme active we use a different op for the URL options.
   */
  function theme($item, $delta) {
    switch ($delta) {
      case 'active':
        $path = $this->environment->getURLPath();
        $options = $this->environment->getURLOptions('active', $this->name, $item['id']);
        $item['link'] = l(t('remove'), $path, $options);
        return theme('searchlight_facet_active', $this->field, $item);
    }
  }

  /**
   * Override of render().
   * The SearchQuery is only rendered active and when a search query is present
   * on the current page query.
   */
  function render($query, $delta) {
    if ($delta === 'active' && !empty($query->search_query)) {
      return array(array(
        'id' => $query->search_query,
        'title' => $query->search_query,
      ));
    }
    return array();
  }

  /**
   * Override of optionsDefault().
   */
  function optionsDefault() {
    $options = parent::optionsDefault();
    $options['items'] = 1;
    return $options;
  }

  /**
   * Override of optionsForm().
   */
  function optionsForm(&$form, $form_state) {
    parent::optionsForm($form, $form_state);
    $form['items'] = array(
      '#type' => 'value',
      '#value' => 1,
    );
  }
}
