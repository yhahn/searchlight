<?php print '<?xml version="1.0" encoding="UTF-8" ?>'; ?>

<!--
 All (relative) paths are relative to the installation path
  
  sharedLib: path to a lib directory that will be shared across all cores
-->
<solr persistent="false">
  <cores>
    <?php foreach ($cores as $core): ?>
      <core name="<?php print $core; ?>" instanceDir="<?php print $core; ?>" />
    <?php endforeach; ?>
  </cores>
</solr>
