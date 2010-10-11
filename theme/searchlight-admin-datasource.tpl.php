<div class='searchlight-admin-datasource'>
  <div class='clearfix'>
    <div class='searchlight-admin-info datasource-info'>
      <h2 class='searchlight-admin-name'><?php print $name ?></h2>
      <label><?php print t('Base table') ?></label>
      <?php print $base_table ?>
    </div>
    <div class='searchlight-admin-info datasource-index'>
      <label><?php print t('Index status') ?></label>
      <strong><?php print drupal_render($form['index']['percent']) ?></strong>
      <small>(<?php print drupal_render($form['index']['counts']) ?>)</small>
      <div class='invalidate'>
        <?php print drupal_render($form['index']['invalidate']) ?>
      </div>
    </div>
  </div>
  <?php print drupal_render($form['help']) ?>
  <?php print drupal_render($form['fields']) ?>
  <?php print drupal_render($form['multivalues']) ?>
  <?php print drupal_render($form['options']) ?>
  <?php print drupal_render_children($form) ?>
</div>
