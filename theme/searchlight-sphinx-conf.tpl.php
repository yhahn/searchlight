
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
