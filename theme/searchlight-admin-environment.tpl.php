<div class='searchlight-admin-environment'>
  <div class='clearfix'>
    <div class='searchlight-admin-info environment-info'>
      <h2 class='searchlight-admin-name'><?php print $name ?></h2>
    </div>
  </div>
  <?php print drupal_render($form['options']) ?>
  <?php print $facets ?>
  <?php print $settings ?>
  <?php print drupal_render_children($form); ?>
</div>
