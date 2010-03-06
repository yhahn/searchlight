<?php
// $Id$

/**
 * Implementation of hook_drush_command().
 */
function sphinx_views_drush_command() {
  $items = array();
  $items['sphinx-conf'] = array(
    'callback' => 'sphinx_views_drush_conf',
    'description' => 'Set the Sphinx configuration for the current site.',
  );
  $items['sphinx-index'] = array(
    'callback' => 'sphinx_views_drush_index',
    'description' => 'Re-generate the Sphinx index for the current site.',
  );
  $items['sphinx-searchd'] = array(
    'callback' => 'sphinx_views_drush_searchd',
    'description' => 'Start a Sphinx searchd daemon for the current site.',
  );
  return $items;
}

/**
 * Implementation of hook_drush_help().
 */
function sphinx_views_drush_help($section) {
  switch ($section) {
    case 'drush:sphinx-conf':
      return dt('');
    case 'drush:sphinx-index':
      return dt('');
    case 'drush:sphinx-searchd':
      return dt('');
  }
}

/**
 * Generate the Sphinx configuration for the current site.
 */
function sphinx_views_drush_conf() {
  $file_path = conf_path() .'/sphinx';
  if (file_check_directory($file_path, TRUE)) {
    // Collect configuration arrays for each datasource.
    $datasources = array();
    foreach (array_keys(variable_get('sphinx_views', array())) as $base_table) {
      if ($datasource = sphinx_views_get_datasource($base_table, TRUE)) {
        $datasources[$base_table] = sphinx_views_datasource_conf($datasource);
      }
    }
    // Generate configuration file from datasources.
    $sphinx_conf = theme('sphinx_views_conf', $datasources, sphinx_views_searchd_conf());
    if ($sphinx_conf) {
      $existing = file_exists("{$file_path}/sphinx.conf") ? file_get_contents("{$file_path}/sphinx.conf") : '';
      if ($sphinx_conf === $existing) {
        return drush_log("{$file_path}/sphinx.conf is unchanged.", 'success');
      }
      else {
        file_put_contents("{$file_path}/sphinx.conf", $sphinx_conf);
        return drush_log("{$file_path}/sphinx.conf was written successfully.", 'success');
      }
    }
  }
  return drush_log("{$file_path}/sphinx.conf could not be written.", 'error');
}

/**
 * Generate new Sphinx indexes for the current site.
 */
function sphinx_views_drush_index() {
  // Create the sphinx directory if it doesn't exist.
  $file_path = conf_path() .'/sphinx/indexes';
  if (file_check_directory($file_path, TRUE)) {
    drush_op('drush_shell_exec', 'indexer --all --config ' . drush_locate_root() .'/'. conf_path() . '/sphinx/sphinx.conf');
    $output = drush_shell_exec_output();
    foreach ($output as $line) {
      drush_print($line);
    }
    drupal_flush_all_caches();
    return drush_log('Indexing complete.', 'success');
  }
  return drush_log("An error ocurred while indexing.", 'error');
}

/**
 * Start or stop the search daemon.
 */
function sphinx_views_drush_searchd() {
  $file_path = conf_path() .'/sphinx/log';
  if (file_check_directory($file_path, TRUE)) {
    drush_op('drush_shell_exec', 'searchd --nodetach -c ' . drush_locate_root() .'/'. conf_path() . '/sphinx/sphinx.conf');
    return drush_log('Search daemon started.', 'success');
  }
  return drush_log("An error ocurred while starting the search daemon.", 'error');
}

/**
 * Generate an array representing the conf and index settings for a datasource.
 */
function sphinx_views_datasource_conf($datasource) {
  $conf = array('conf' => array(), 'index' => array());

  // Retrieve db info.
  global $db_url, $db_type;
  $url = is_array($db_url) ? $db_url['default'] : $db_url;
  $url = parse_url($url);

  // Configuration options.
  $conf['conf']['id'] = sphinx_views_get_datasource_id($datasource['base_table']);
  $conf['conf']['type'] = $db_type;
  $conf['conf']['sql_user'] = urldecode($url['user']);
  $conf['conf']['sql_pass'] = isset($url['pass']) ? urldecode($url['pass']) : '';
  $conf['conf']['sql_host'] = urldecode($url['host']);
  $conf['conf']['sql_db']   = trim(urldecode($url['path']), '/');
  $conf['conf']['sql_port'] = isset($url['port']) ? urldecode($url['port']) : '3306';
  $conf['conf']['sql_query'] = str_replace("\n", " \\\n", trim($datasource['conf']['sql_query']));
  $conf['conf']['sql_query_info'] = str_replace("\n", " \\\n", trim($datasource['conf']['sql_query_info']));
  $conf['conf']['sql_query_range'] = str_replace("\n", " \\\n", trim($datasource['conf']['sql_query_range']));
  $conf['conf']['sql_range_step'] = 1000;
  $conf['conf']['sql_ranged_throttle'] = 0;

  // Merge in attributes.
  $sql_attr = array();
  foreach ($datasource['attributes'] as $attribute) {
    if ($attribute['type'] === 'sql_attr_multi') {
      $query = str_replace("\n", " \\\n", trim($attribute['mva_query']));
      $sql_attr[] = "{$attribute['type']} = {$attribute['mva_type']} {$attribute['name']} from {$attribute['mva_source']}; {$query}";
    }
    else {
      $sql_attr[] = "{$attribute['type']} = {$attribute['name']}";
    }
  }
  $sql_attr = array_unique($sql_attr);
  $conf['conf']['sql_attr'] = implode(" \n", $sql_attr);

  // Index information
  $conf['index']['path'] = drush_locate_root() .'/'. conf_path() . '/sphinx/indexes/' . sphinx_views_get_datasource_id($datasource['base_table']);
  $conf['index']['docinfo'] = 'extern';
  $conf['index']['mlock'] = 0;
  $conf['index']['morphology'] = 'stem_en';
  $conf['index']['min_word_len'] = 1;
  $conf['index']['charset_type'] = 'utf-8';
  $conf['index']['html_strip'] = 0;

  return $conf;
}

/**
 * Get the Sphinx searchd settings.
 */
function sphinx_views_searchd_conf() {
  $searchd = array();
  $searchd['log'] = drush_locate_root() .'/'. conf_path() . '/sphinx/log/searchd.log';
  $searchd['query_log'] = drush_locate_root() .'/'. conf_path() . '/sphinx/log/query.log';
  $searchd['pid_file'] = drush_locate_root() .'/'. conf_path() . '/sphinx/log/searchd.pid';
  return $searchd;
}
