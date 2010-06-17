<?php
// $Id$

function drush_searchlight_provision_post_provision_install($url) {
  if (module_exists('searchlight')) {
    if ($backend = searchlight_get_backend()) {
      $backend->drushAegirInstall();
    }
  }
}

function drush_searchlight_provision_post_provision_deploy($url) {
  if (module_exists('searchlight')) {
    if ($backend = searchlight_get_backend()) {
      $backend->drushAegirDeploy();
    }
  }
}

