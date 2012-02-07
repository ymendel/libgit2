static void file_delta_free(git_diff_delta *delta)
{
	if (!delta)
		return;

	if (delta->new_path != delta->path) {
		git__free((char *)delta->new_path);
		delta->new_path = NULL;
	}

	git__free((char *)delta->path);
	delta->path = NULL;

	git__free(delta);
}

static int file_delta_new__from_one(
	git_status_t status,
	unsigned int attr,
	const git_oid *oid,
	const char *path)
	int error;
	/* This fn is just for single-sided diffs */
	assert(status == GIT_STATUS_ADDED || status == GIT_STATUS_DELETED);

	if (!delta)
		return git__rethrow(GIT_ENOMEM, "Could not allocate diff record");

	if ((delta->path = git__strdup(path)) == NULL) {
		git__free(delta);
		return git__rethrow(GIT_ENOMEM, "Could not allocate diff record path");
	if (diff->opts.flags & GIT_DIFF_REVERSE)
		status = (status == GIT_STATUS_ADDED) ?
			GIT_STATUS_DELETED : GIT_STATUS_ADDED;
	delta->status = status;

	if (status == GIT_STATUS_ADDED) {
		delta->new_attr = attr;
		git_oid_cpy(&delta->new_oid, oid);
	} else {
		delta->old_attr = attr;
		git_oid_cpy(&delta->old_oid, oid);
	}

	if ((error = git_vector_insert(&diff->files, delta)) < GIT_SUCCESS)
		file_delta_free(delta);

	return error;
}

static int file_delta_new__from_tree_diff(
	git_diff_list *diff,
	const git_tree_diff_data *tdiff)
{
	int error;
	git_diff_delta *delta = git__calloc(1, sizeof(git_diff_delta));

	if (!delta)
		return git__rethrow(GIT_ENOMEM, "Could not allocate diff record");

	if ((diff->opts.flags & GIT_DIFF_REVERSE) == 0) {
		delta->status   = tdiff->status;
		delta->old_attr = tdiff->old_attr;
		delta->new_attr = tdiff->new_attr;
		delta->old_oid  = tdiff->old_oid;
		delta->new_oid  = tdiff->new_oid;
	} else {
		/* reverse the polarity of the neutron flow */
		switch (tdiff->status) {
		case GIT_STATUS_ADDED:   delta->status = GIT_STATUS_DELETED; break;
		case GIT_STATUS_DELETED: delta->status = GIT_STATUS_ADDED; break;
		default:                 delta->status = tdiff->status;
		}
		delta->old_attr = tdiff->new_attr;
		delta->new_attr = tdiff->old_attr;
		delta->old_oid  = tdiff->new_oid;
		delta->new_oid  = tdiff->old_oid;
	}

	delta->path = git__strdup(diff->pfx.ptr);
	if (delta->path == NULL) {
		return git__rethrow(GIT_ENOMEM, "Could not allocate diff record path");
	if ((error = git_vector_insert(&diff->files, delta)) < GIT_SUCCESS)
		file_delta_free(delta);

	return error;
static int tree_walk_cb(const char *root, git_tree_entry *entry, void *data)
	ssize_t pfx_len = diff->pfx.size;
	if (S_ISDIR(git_tree_entry_attributes(entry)))
		return GIT_SUCCESS;
	/* join pfx, root, and entry->filename into one */
	if ((error = git_buf_joinpath(&diff->pfx, diff->pfx.ptr, root)) ||
		(error = git_buf_joinpath(
			&diff->pfx, diff->pfx.ptr, git_tree_entry_name(entry))))
		return error;

	error = file_delta_new__from_one(
		diff, diff->mode, git_tree_entry_attributes(entry),
		git_tree_entry_id(entry), diff->pfx.ptr);

	git_buf_truncate(&diff->pfx, pfx_len);

	return error;
}

static int tree_diff_cb(const git_tree_diff_data *tdiff, void *data)
{
	int error;
	git_diff_list *diff = data;
	ssize_t pfx_len = diff->pfx.size;

	error = git_buf_joinpath(&diff->pfx, diff->pfx.ptr, tdiff->path);
	if (error < GIT_SUCCESS)
		return error;

	/* there are 4 tree related cases:
	 * - diff tree to tree, which just means we recurse
	 * - tree was deleted
	 * - tree was added
	 * - tree became non-tree or vice versa, which git_tree_diff
	 *   will already have converted into two calls: an addition
	 *   and a deletion (thank you, git_tree_diff!)
	 * otherwise, this is a blob-to-blob diff
	 */
	if (S_ISDIR(tdiff->old_attr) && S_ISDIR(tdiff->new_attr)) {
		if (!(error = git_tree_lookup(&old, diff->repo, &tdiff->old_oid)) &&
			!(error = git_tree_lookup(&new, diff->repo, &tdiff->new_oid)))
	} else if (S_ISDIR(tdiff->old_attr) || S_ISDIR(tdiff->new_attr)) {
		git_tree *tree     = NULL;
		int added_dir      = S_ISDIR(tdiff->new_attr);
		const git_oid *oid = added_dir ? &tdiff->new_oid : &tdiff->old_oid;
		diff->mode         = added_dir ? GIT_STATUS_ADDED : GIT_STATUS_DELETED;

		if (!(error = git_tree_lookup(&tree, diff->repo, oid)))
			error = git_tree_walk(tree, tree_walk_cb, GIT_TREEWALK_POST, diff);
		git_tree_free(tree);
	} else
		error = file_delta_new__from_tree_diff(diff, tdiff);

	git_buf_truncate(&diff->pfx, pfx_len);
static char *git_diff_src_prefix_default = "a/";
static char *git_diff_dst_prefix_default = "b/";
#define PREFIX_IS_DEFAULT(A) \
	((A) == git_diff_src_prefix_default || (A) == git_diff_dst_prefix_default)

static char *copy_prefix(const char *prefix)
{
	size_t len = strlen(prefix);
	char *str = git__malloc(len + 2);
	if (str != NULL) {
		memcpy(str, prefix, len + 1);
		/* append '/' at end if needed */
		if (len > 0 && str[len - 1] != '/') {
			str[len] = '/';
			str[len + 1] = '\0';
		}
	}
	return str;
}

	if (diff == NULL)
		return NULL;

	diff->repo = repo;
	git_buf_init(&diff->pfx, 0);

	if (opts == NULL)
		return diff;

	memcpy(&diff->opts, opts, sizeof(git_diff_options));

	diff->opts.src_prefix = (opts->src_prefix == NULL) ?
		git_diff_src_prefix_default : copy_prefix(opts->src_prefix);
	diff->opts.dst_prefix = (opts->dst_prefix == NULL) ?
		git_diff_dst_prefix_default : copy_prefix(opts->dst_prefix);
	if (!diff->opts.src_prefix || !diff->opts.dst_prefix) {
		git__free(diff);
		return NULL;
	if (diff->opts.flags & GIT_DIFF_REVERSE) {
		char *swap = diff->opts.src_prefix;
		diff->opts.src_prefix = diff->opts.dst_prefix;
		diff->opts.dst_prefix = swap;
	}

	/* do something safe with the pathspec strarray */

	git_diff_delta *delta;
	unsigned int i;


	git_vector_foreach(&diff->files, i, delta) {
		file_delta_free(delta);
		diff->files.contents[i] = NULL;
	}
	git_vector_free(&diff->files);
	if (!PREFIX_IS_DEFAULT(diff->opts.src_prefix)) {
		git__free(diff->opts.src_prefix);
		diff->opts.src_prefix = NULL;
	}
	if (!PREFIX_IS_DEFAULT(diff->opts.dst_prefix)) {
		git__free(diff->opts.dst_prefix);
		diff->opts.dst_prefix = NULL;
	}
typedef struct {
	git_diff_list *diff;
	git_index *index;
	unsigned int index_pos;
} index_to_tree_info;

static int add_new_index_deltas(
	index_to_tree_info *info,
	const char *stop_path)
{
	int error;
	git_index_entry *idx_entry = git_index_get(info->index, info->index_pos);

	while (idx_entry != NULL &&
		(stop_path == NULL || strcmp(idx_entry->path, stop_path) < 0))
	{
		error = file_delta_new__from_one(
			info->diff, GIT_STATUS_ADDED, idx_entry->mode,
			&idx_entry->oid, idx_entry->path);
		if (error < GIT_SUCCESS)
			return error;

		idx_entry = git_index_get(info->index, ++info->index_pos);
	}

	return GIT_SUCCESS;
}

static int diff_index_to_tree_cb(const char *root, git_tree_entry *tree_entry, void *data)
{
	int error;
	index_to_tree_info *info = data;
	git_index_entry *idx_entry;

	/* TODO: submodule support for GIT_OBJ_COMMITs in tree */
	if (git_tree_entry_type(tree_entry) != GIT_OBJ_BLOB)
		return GIT_SUCCESS;

	error = git_buf_joinpath(&info->diff->pfx, root, git_tree_entry_name(tree_entry));
	if (error < GIT_SUCCESS)
		return error;

	/* create add deltas for index entries that are not in the tree */
	error = add_new_index_deltas(info, info->diff->pfx.ptr);
	if (error < GIT_SUCCESS)
		return error;

	/* create delete delta for tree entries that are not in the index */
	idx_entry = git_index_get(info->index, info->index_pos);
	if (idx_entry == NULL || strcmp(idx_entry->path, info->diff->pfx.ptr) > 0) {
		return file_delta_new__from_one(
			info->diff, GIT_STATUS_DELETED, git_tree_entry_attributes(tree_entry),
			git_tree_entry_id(tree_entry), info->diff->pfx.ptr);
	}

	/* create modified delta for non-matching tree & index entries */
	info->index_pos++;

	if (git_oid_cmp(&idx_entry->oid, git_tree_entry_id(tree_entry)) ||
		idx_entry->mode != git_tree_entry_attributes(tree_entry))
	{
		git_tree_diff_data tdiff;
		tdiff.old_attr = git_tree_entry_attributes(tree_entry);
		tdiff.new_attr = idx_entry->mode;
		tdiff.status   = GIT_STATUS_MODIFIED;
		tdiff.path     = idx_entry->path;
		git_oid_cpy(&tdiff.old_oid, git_tree_entry_id(tree_entry));
		git_oid_cpy(&tdiff.new_oid, &idx_entry->oid);

		error = file_delta_new__from_tree_diff(info->diff, &tdiff);
	}

	return error;

}

int git_diff_index_to_tree(
	git_repository *repo,
	const git_diff_options *opts,
	git_tree *old,
	git_diff_list **diff_ptr)
{
	int error;
	index_to_tree_info info = {0};

	if ((info.diff = git_diff_list_alloc(repo, opts)) == NULL)
		return GIT_ENOMEM;

	if ((error = git_repository_index(&info.index, repo)) == GIT_SUCCESS) {
		error = git_tree_walk(
			old, diff_index_to_tree_cb, GIT_TREEWALK_POST, &info);
		if (error == GIT_SUCCESS)
			error = add_new_index_deltas(&info, NULL);
		git_index_free(info.index);
	}
	git_buf_free(&info.diff->pfx);

	if (error != GIT_SUCCESS)
		git_diff_list_free(info.diff);
	else
		*diff_ptr = info.diff;

	return error;
}

static void setup_xdiff_config(git_diff_options *opts, xdemitconf_t *cfg)
{
	memset(cfg, 0, sizeof(xdemitconf_t));

	cfg->ctxlen =
		(!opts || !opts->context_lines) ? 3 : opts->context_lines;
	cfg->interhunkctxlen =
		(!opts || !opts->interhunk_lines) ? 3 : opts->interhunk_lines;

	if (!opts)
		return;

	if (opts->flags & GIT_DIFF_IGNORE_WHITESPACE)
		cfg->flags |= XDF_WHITESPACE_FLAGS;
	if (opts->flags & GIT_DIFF_IGNORE_WHITESPACE_CHANGE)
		cfg->flags |= XDF_IGNORE_WHITESPACE_CHANGE;
	if (opts->flags & GIT_DIFF_IGNORE_WHITESPACE_EOL)
		cfg->flags |= XDF_IGNORE_WHITESPACE_AT_EOL;
}

	xpparam_t    xdiff_params;
	xdemitconf_t xdiff_config;
	xdemitcb_t   xdiff_callback;
	memset(&xdiff_params, 0, sizeof(xdiff_params));
	setup_xdiff_config(&diff->opts, &xdiff_config);
	memset(&xdiff_callback, 0, sizeof(xdiff_callback));
	xdiff_callback.outf = diff_output_cb;
	xdiff_callback.priv = &di;

		mmfile_t old_data, new_data;
				old_data.ptr = (char *)git_blob_rawcontent(delta->old_blob);
				old_data.size = git_blob_rawsize(delta->old_blob);
				old_data.ptr = "";
				old_data.size = 0;
				new_data.ptr = (char *)git_blob_rawcontent(delta->new_blob);
				new_data.size = git_blob_rawsize(delta->new_blob);
				new_data.ptr = "";
				new_data.size = 0;
		if (diff->opts.flags & GIT_DIFF_FORCE_TEXT)
			diff->repo, delta, &old_data, &new_data)) < GIT_SUCCESS)
		/* TODO: if ignore_whitespace is set, then we *must* do text
		 * diffs to tell if a file has really been changed.
		 */

		xdl_diff(&old_data, &new_data,
			&xdiff_params, &xdiff_config, &xdiff_callback);
typedef struct {
	git_diff_list *diff;
	git_diff_output_fn print_cb;
	void *cb_data;
	git_buf *buf;
} print_info;
	else if (mode & 0100)
		/* modes in git are not very flexible, so if this bit is set,
		 * we must be dealwith with a 100755 type of file.
		 */
	print_info *pi = data;
	git_buf_clear(pi->buf);

		git_buf_printf(pi->buf, "%c\t%s%c -> %s%c\n", code,
			delta->path, old_suffix, delta->new_path, new_suffix);
	else if (delta->old_attr != delta->new_attr &&
		delta->old_attr != 0 && delta->new_attr != 0)
		git_buf_printf(pi->buf, "%c\t%s%c (%o -> %o)\n", code,
			delta->path, new_suffix, delta->old_attr, delta->new_attr);
	else if (old_suffix != ' ')
		git_buf_printf(pi->buf, "%c\t%s%c\n", code, delta->path, old_suffix);
		git_buf_printf(pi->buf, "%c\t%s\n", code, delta->path);
	if (git_buf_lasterror(pi->buf) != GIT_SUCCESS)
		return git_buf_lasterror(pi->buf);

	return pi->print_cb(pi->cb_data, GIT_DIFF_LINE_FILE_HDR, pi->buf->ptr);
int git_diff_print_compact(
	git_diff_list *diff,
	void *cb_data,
	git_diff_output_fn print_cb)
	int error;
	git_buf buf = GIT_BUF_INIT;
	print_info pi;

	pi.diff     = diff;
	pi.print_cb = print_cb;
	pi.cb_data  = cb_data;
	pi.buf      = &buf;

	error = git_diff_foreach(diff, &pi, print_compact, NULL, NULL);

	git_buf_free(&buf);

	return error;

static int print_oid_range(print_info *pi, git_diff_delta *delta)
	char start_oid[8], end_oid[8];


	/* TODO: Match git diff more closely */
	if (delta->old_attr == delta->new_attr) {
		git_buf_printf(pi->buf, "index %s..%s %o\n",
	} else {
		if (delta->old_attr == 0) {
			git_buf_printf(pi->buf, "new file mode %o\n", delta->new_attr);
		} else if (delta->new_attr == 0) {
			git_buf_printf(pi->buf, "deleted file mode %o\n", delta->old_attr);
		} else {
			git_buf_printf(pi->buf, "old mode %o\n", delta->old_attr);
			git_buf_printf(pi->buf, "new mode %o\n", delta->new_attr);
		}
		git_buf_printf(pi->buf, "index %s..%s\n", start_oid, end_oid);
	}

	return git_buf_lasterror(pi->buf);
	int error;
	print_info *pi = data;
	const char *oldpfx = pi->diff->opts.src_prefix;
	const char *oldpath = delta->path;
	const char *newpfx = pi->diff->opts.dst_prefix;
	git_buf_clear(pi->buf);
	git_buf_printf(pi->buf, "diff --git %s%s %s%s\n", oldpfx, delta->path, newpfx, newpath);
	if ((error = print_oid_range(pi, delta)) < GIT_SUCCESS)
		return error;

	if (delta->old_blob == NULL) {
		oldpfx = "";
		oldpath = "/dev/null";
	}
	if (delta->new_blob == NULL) {
		oldpfx = "";
		oldpath = "/dev/null";
	if (!delta->binary) {
		git_buf_printf(pi->buf, "--- %s%s\n", oldpfx, oldpath);
		git_buf_printf(pi->buf, "+++ %s%s\n", newpfx, newpath);
	}

	if (git_buf_lasterror(pi->buf) != GIT_SUCCESS)
		return git_buf_lasterror(pi->buf);

	error = pi->print_cb(pi->cb_data, GIT_DIFF_LINE_FILE_HDR, pi->buf->ptr);
	if (error != GIT_SUCCESS || !delta->binary)
		return error;

	git_buf_clear(pi->buf);
	git_buf_printf(
		pi->buf, "Binary files %s%s and %s%s differ\n",
		oldpfx, oldpath, newpfx, newpath);
	if (git_buf_lasterror(pi->buf) != GIT_SUCCESS)
		return git_buf_lasterror(pi->buf);

	return pi->print_cb(pi->cb_data, GIT_DIFF_LINE_BINARY, pi->buf->ptr);
	print_info *pi = data;


	git_buf_clear(pi->buf);

	if (git_buf_printf(pi->buf, "%.*s", (int)header_len, header) == GIT_SUCCESS)
		return pi->print_cb(pi->cb_data, GIT_DIFF_LINE_HUNK_HDR, pi->buf->ptr);
	else
		return git_buf_lasterror(pi->buf);
	print_info *pi = data;


	git_buf_clear(pi->buf);

	if (line_origin == GIT_DIFF_LINE_ADDITION ||
		line_origin == GIT_DIFF_LINE_DELETION ||
		line_origin == GIT_DIFF_LINE_CONTEXT)
		git_buf_printf(pi->buf, "%c%.*s", line_origin, (int)content_len, content);
		git_buf_printf(pi->buf, "%.*s", (int)content_len, content);

	if (git_buf_lasterror(pi->buf) != GIT_SUCCESS)
		return git_buf_lasterror(pi->buf);

	return pi->print_cb(pi->cb_data, line_origin, pi->buf->ptr);
int git_diff_print_patch(
	git_diff_list *diff,
	void *cb_data,
	git_diff_output_fn print_cb)
	int error;
	git_buf buf = GIT_BUF_INIT;
	print_info pi;

	pi.diff     = diff;
	pi.print_cb = print_cb;
	pi.cb_data  = cb_data;
	pi.buf      = &buf;

	error = git_diff_foreach(
		diff, &pi, print_patch_file, print_patch_hunk, print_patch_line);

	git_buf_free(&buf);

	return error;
	xpparam_t xdiff_params;
	xdemitconf_t xdiff_config;
	xdemitcb_t xdiff_callback;

	assert(repo);
	if (options && (options->flags & GIT_DIFF_REVERSE)) {
		git_blob *swap = old_blob;
		old_blob = new_blob;
		new_blob = swap;
	}
	di.line_cb = line_cb;
	memset(&xdiff_params, 0, sizeof(xdiff_params));
	setup_xdiff_config(options, &xdiff_config);
	memset(&xdiff_callback, 0, sizeof(xdiff_callback));
	xdiff_callback.outf = diff_output_cb;
	xdiff_callback.priv = &di;
	xdl_diff(&old, &new, &xdiff_params, &xdiff_config, &xdiff_callback);