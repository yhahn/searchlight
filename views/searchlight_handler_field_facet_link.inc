<?php

class searchlight_handler_field_facet_link extends views_handler_field {
  function query() { }

  function allow_advanced_render() {
    return FALSE;
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['exclude']['default'] = TRUE;
    $options['facet']['default'] = NULL;
    $options['field_token']['default'] = NULL;
    $options['text_token']['default'] = NULL;
    $options['absolute']['default'] = FALSE;
    return $options;
  }

  function options_form(&$form, $form_state) {
    $options = array();
    foreach (searchlight_environment_load() as $environment) {
      foreach ($environment->getFacets() as $facet => $info) {
        $options[$environment->name]["{$environment->name}:{$facet}"] = $facet;
      }
    }
    $form['help'] = array(
      '#type' => 'item',
      '#value' => t('Generate a facet URL that can be used by other Views fields to link to an active facet in a search environment.'),
    );
    $form['facet'] = array(
      '#title' => t('Facet'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $this->options['facet'],
    );
    $form['field_token'] = array(
      '#title' => t('Value'),
      '#type' => 'select',
      '#options' => array(0 => '---') + drupal_map_assoc(array_keys($this->get_render_tokens(array()))),
      '#default_value' => $this->options['field_token'],
    );
    $form['text_token'] = array(
      '#title' => t('Display as link'),
      '#type' => 'select',
      '#options' => array(0 => '---') + drupal_map_assoc(array_keys($this->get_render_tokens(array()))),
      '#default_value' => $this->options['text_token'],
    );
    $form['absolute'] = array(
      '#title' => t('Absolute URL'),
      '#type' => 'checkbox',
      '#default_value' => $this->options['absolute'],
    );
    $form['exclude'] = array(
      '#title' => t('Exclude this field'),
      '#type' => 'checkbox',
      '#default_value' => $this->options['exclude'],
    );
  }

  function render($values) {
    if (!empty($this->options['facet'])) {
      list($environment, $facet) = explode(':', $this->options['facet']);
      if ($environment = searchlight_environment_load($environment)) {
        $tokens = $this->get_render_tokens(array());
        $path = $environment->getURLPath();
        $options = $environment->getURLOptions('add', $facet, $tokens[$this->options['field_token']]);
        $options['absolute'] = $this->options['absolute'];
        // TODO The second parameter to this function call should be an array.
        if (!empty($this->options['text_token'])) {
          return l(check_plain($tokens[$this->options['text_token']]), $path, $options);
        }
        else {
          return url($path, $options);
        }
      }
    }
    return '';
  }
}
