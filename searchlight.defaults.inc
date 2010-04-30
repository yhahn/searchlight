<?php

/**
 * Helper to implementation of hook_searchlight_default_datasources().
 */
function _searchlight_searchlight_default_datasources() {
  $export = array();
  $searchlight_datasource = new SearchlightDatasource;
  $searchlight_datasource->disabled = FALSE; /* Edit this to true to make a default searchlight_datasource disabled initially */
  $searchlight_datasource->api_version = 1;
  $searchlight_datasource->name = 'node';
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
    'node_revisions_body' => array(
      'label' => 'Node: Body (body)',
      'datatype' => 'text',
      'table' => 'node_revisions',
      'field' => 'body',
      'name' => 'node_revisions_body',
      'usage' => 'content',
    ),
    'node_revisions_format' => array(
      'label' => 'Node: Body (format)',
      'datatype' => 'int',
      'table' => 'node_revisions',
      'field' => 'format',
      'name' => 'node_revisions_format',
      'usage' => 'attribute',
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
  );
  $searchlight_datasource->filters = FALSE;
  $searchlight_datasource->options = array(
    'node_access' => 1,
  );

  $export['node'] = $searchlight_datasource;
  return $export;
}