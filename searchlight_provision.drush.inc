<?php
// $Id$

/**
 * Post-hook implementation for `provision-install` command.
 */
function drush_searchlight_provision_post_provision_install() {
  if (module_exists('searchlight') && $backend = searchlight_get_backend()) {
    $backend->drushAegirInstall();
  }
}

/**
 * Post-hook implementation for `provision-deploy` command.
 */
function drush_searchlight_provision_post_provision_deploy() {
  if (module_exists('searchlight') && $backend = searchlight_get_backend()) {
    $backend->drushAegirDeploy();
  }
}
