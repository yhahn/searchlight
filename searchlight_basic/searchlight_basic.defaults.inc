<?php

/**
 * Helper to implementation of hook_ctools_plugin_api().
 */
function _searchlight_basic_ctools_plugin_api() {
  $args = func_get_args();
  $module = array_shift($args);
  $api = array_shift($args);
  if ($module == "searchlight" && $api == "datasource") {
    return array("version" => 1);
  }
  else if ($module == "searchlight" && $api == "environment") {
    return array("version" => 1);
  }
}

/**
 * Helper to implementation of hook_searchlight_default_datasources().
 */
function _searchlight_basic_searchlight_default_datasources() {
  $export = array();
  $searchlight_datasource = new SearchlightDatasource;
  $searchlight_datasource->disabled = FALSE; /* Edit this to true to make a default searchlight_datasource disabled initially */
  $searchlight_datasource->api_version = 1;
  $searchlight_datasource->name = 'search_node';
  $searchlight_datasource->base_table = 'node';
  $searchlight_datasource->fields = array(
    'node_title' => array(
      'label' => 'Node: Title (title)',
      'datatype' => 'text',
      'table' => 'node',
      'field' => 'title',
      'name' => 'node_title',
      'usage' => 'content',
    ),
    'node_status' => array(
      'label' => 'Node: Published (status)',
      'datatype' => 'int',
      'table' => 'node',
      'field' => 'status',
      'name' => 'node_status',
      'usage' => 'attribute',
    ),
    'node_created' => array(
      'label' => 'Node: Post date (created)',
      'datatype' => 'timestamp',
      'table' => 'node',
      'field' => 'created',
      'name' => 'node_created',
      'usage' => 'attribute',
    ),
    'users_name' => array(
      'label' => 'User: Name (name)',
      'datatype' => 'text',
      'table' => 'users',
      'field' => 'name',
      'name' => 'users_name',
      'usage' => 'content',
    ),
    'users_uid' => array(
      'label' => 'User: Name (uid)',
      'datatype' => 'int',
      'table' => 'users',
      'field' => 'uid',
      'name' => 'users_uid',
      'usage' => 'attribute',
    ),
    'node_type' => array(
      'label' => 'Node: Type (type)',
      'datatype' => 'text',
      'table' => 'node',
      'field' => 'type',
      'name' => 'node_type',
      'usage' => 'attribute',
    ),
    'field_data_body_entity_id' => array(
      'label' => 'Fields: body (entity_id)',
      'datatype' => 'int',
      'table' => 'field_data_body',
      'field' => 'entity_id',
      'name' => 'field_data_body_entity_id',
      'usage' => 'attribute',
    ),
    'field_data_body_body_value' => array(
      'label' => 'Fields: body (body_value)',
      'datatype' => 'text',
      'table' => 'field_data_body',
      'field' => 'body_value',
      'name' => 'field_data_body_body_value',
      'usage' => 'content',
    ),
  );
  $searchlight_datasource->filters = array();
  $searchlight_datasource->options = array(
    'node_access' => 1,
  );

  $export['search_node'] = $searchlight_datasource;
  return $export;
}

/**
 * Helper to implementation of hook_searchlight_default_environments().
 */
function _searchlight_basic_searchlight_default_environments() {
  $export = array();
  $searchlight_environment = new SearchlightEnvironment;
  $searchlight_environment->disabled = FALSE; /* Edit this to true to make a default searchlight_environment disabled initially */
  $searchlight_environment->api_version = 1;
  $searchlight_environment->name = 'search_node';
  $searchlight_environment->view_display = 'search_node:page_1';
  $searchlight_environment->facets = array(
    'node_status' => array(
      'enabled' => 0,
      'weight' => '-6',
      'label' => '',
      'items' => '5',
    ),
    'node_created' => array(
      'enabled' => 1,
      'weight' => '-9',
      'label' => '',
      'items' => '5',
      'granularity' => 'month',
      'date_format' => 'F, Y',
    ),
    'users_name' => array(
      'enabled' => 1,
      'weight' => '-7',
      'label' => '',
      'items' => '5',
    ),
    'users_uid' => array(
      'enabled' => 0,
      'weight' => '-5',
      'label' => '',
      'items' => '5',
    ),
    'node_type' => array(
      'enabled' => 1,
      'weight' => '-8',
      'label' => '',
      'items' => '5',
    ),
    'search_query' => array(
      'enabled' => 1,
      'weight' => '-10',
      'label' => '',
      'items' => 1,
    ),
  );
  $searchlight_environment->options = array(
    'active_label' => 'Active search',
    'facets_label' => 'Search facets',
  );

  $export['search_node'] = $searchlight_environment;
  return $export;
}
