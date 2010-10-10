<?php

class searchlight_handler_field_node_access extends views_handler_field {
  function option_definition() {
    $options = parent::option_definition();
    $options['realm'] = array('default' => NULL);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $options = array();
    $result = db_query("SELECT realm FROM {node_access} GROUP BY realm");
    foreach ($result as $row) {
      $options[$row->realm] = check_plain($row->realm);
    }
    $form['realm'] = array(
      '#type' => 'select',
      '#options' => $options,
    );
  }

  function construct() {
    parent::construct();
    $this->additional_fields[] = 'gid';
  }

  function query() {
    parent::query();
    $table = $this->ensure_my_table();
    if (!empty($this->options['realm'])) {
      $this->query->add_where('AND', "$table.realm = '%s'", $this->options['realm']);
      $this->query->add_where('AND', "$table.grant_view >= 1");
    }
  }

  /**
   * Override of add_additional_fields().
   * Use field aliases specific to the realm specified in this view.
   */
  function add_additional_fields($fields = NULL) {
    $fields = !isset($fields) ? $this->additional_fields : $fields;
    foreach ($fields as $identifier => $info) {
      $this->aliases[$info] = $this->query->add_field($this->table_alias, $info, "{$this->table_alias}_{$info}_{$this->options['realm']}");
    }
  }

  function allow_advanced_render() {
    return FALSE;
  }
}
