<?php foreach ($datasources as $datasource): ?>
#############################################################################
# datasource: <?php print $datasource['conf']['id'] ?>
#############################################################################

source <?php print $datasource['conf']['id'] ?>
{
type = <?php print $datasource['conf']['type'] ?>
sql_host = <?php print $datasource['conf']['sql_host'] ?>
sql_user = <?php print $datasource['conf']['sql_user'] ?>
sql_pass = <?php print $datasource['conf']['sql_pass'] ?>
sql_db   = <?php print $datasource['conf']['sql_db'] ?>
sql_port = <?php print $datasource['conf']['sql_port'] ?>

sql_query = \
  <?php print $datasource['conf']['sql_query'] ?>
sql_query_info = \
  <?php print $datasource['conf']['sql_query_info'] ?>
sql_query_range = <?php print $datasource['conf']['sql_query_range'] ?>
sql_range_step = <?php print $datasource['conf']['sql_range_step'] ?>
sql_ranged_throttle = <?php print $datasource['conf']['sql_ranged_throttle'] ?>

<?php print $datasource['conf']['sql_attr'] ?>
}

#############################################################################
# index: <?php print $datasource['conf']['id'] ?>
#############################################################################

index <?php print $datasource['conf']['id'] ?>
{
# Index configuration
source = <?php print $datasource['conf']['id'] ?>
<?php foreach (element_children($datasource['index']) as $key): ?>
<?php print $key ?> = <?php print $datasource['index'][$key] ?>
<?php endforeach; ?>
} 

<?php endforeach;?>

#############################################################################
# indexer settings
#############################################################################

indexer
{
mem_limit = 32M
}

#############################################################################
# searchd settings
#############################################################################

searchd
{
listen            = 127.0.0.1
log               = <?php print $searchd['log'] ?>
query_log         = <?php print $searchd['query_log'] ?>
read_timeout      = 5
client_timeout    = 300
max_children      = 30
pid_file          = <?php print $searchd['pid_file'] ?>
max_matches       = 1000
seamless_rotate   = 1
preopen_indexes   = 0
unlink_old        = 1
mva_updates_pool  = 1M
max_packet_size   = 8M
max_filters       = 256
max_filter_values = 4096
}
