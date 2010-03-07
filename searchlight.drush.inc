<?php
// $Id$

/**
 * Implementation of hook_drush_command().
 */
function searchlight_drush_command() {
  $items = array();
  $items['searchlight-conf'] = array(
    'callback' => 'searchlight_drush_conf',
    'description' => 'Set the Searchlight configuration for the current site.',
    'drupal dependencies' => array('searchlight'),
  );
  $items['searchlight-index'] = array(
    'callback' => 'searchlight_drush_index',
    'description' => 'Re-generate the Searchlight index for the current site.',
    'drupal dependencies' => array('searchlight'),
  );
  // @TODO
  $items['searchlight-searchd'] = array(
    'callback' => 'searchlight_drush_searchd',
    'description' => 'Start a Sphinx searchd daemon for the current site.',
    'drupal dependencies' => array('searchlight'),
  );
  return $items;
}

/**
 * Generate the configuration file for the current site.
 */
function searchlight_drush_conf() {
  $backend = searchlight_default_backend();
  $backend->drushWriteConf();
}

/**
 * Generate new indexes for the current site.
 */
function searchlight_drush_index() {
  $backend = searchlight_default_backend();
  $backend->drushIndex();
}

/**
 * Start or stop the search daemon.
 */
function searchlight_drush_searchd() {
  $backend = searchlight_default_backend();
  $backend->drushSearchd();
}
