#include "clar_libgit2.h"
#include "diff_helpers.h"

static git_repository *g_repo = NULL;

void test_diff_tree__initialize(void)
{
	cl_fixture_sandbox("attr");
	cl_git_pass(p_rename("attr/.gitted", "attr/.git"));
	cl_git_pass(p_rename("attr/gitattributes", "attr/.gitattributes"));
	cl_git_pass(git_repository_open(&g_repo, "attr/.git"));
}

void test_diff_tree__cleanup(void)
{
	git_repository_free(g_repo);
	g_repo = NULL;
	cl_fixture_cleanup("attr");
}

void test_diff_tree__0(void)
{
	/* grabbed a couple of commit oids from the history of the attr repo */
	const char *a_commit = "605812a";
	const char *b_commit = "370fe9ec22";
	const char *c_commit = "f5b0af1fb4f5c";
	git_tree *a = resolve_commit_oid_to_tree(g_repo, a_commit);
	git_tree *b = resolve_commit_oid_to_tree(g_repo, b_commit);
	git_tree *c = resolve_commit_oid_to_tree(g_repo, c_commit);
	git_diff_options opts = {0};
	git_diff_list *diff = NULL;
	diff_expects exp;

	cl_assert(a);
	cl_assert(b);

	opts.context_lines = 1;
	opts.interhunk_lines = 1;

	memset(&exp, 0, sizeof(exp));

	cl_git_pass(git_diff_tree_to_tree(g_repo, &opts, a, b, &diff));

	cl_git_pass(git_diff_foreach(
		diff, &exp, diff_file_fn, diff_hunk_fn, diff_line_fn));

	cl_assert(exp.files == 5);
	cl_assert(exp.file_adds == 2);
	cl_assert(exp.file_dels == 1);
	cl_assert(exp.file_mods == 2);

	cl_assert(exp.hunks == 5);

	cl_assert(exp.lines == 7 + 24 + 1 + 6 + 6);
	cl_assert(exp.line_ctxt == 1);
	cl_assert(exp.line_adds == 24 + 1 + 5 + 5);
	cl_assert(exp.line_dels == 7 + 1);

	git_diff_list_free(diff);
	diff = NULL;

	memset(&exp, 0, sizeof(exp));

	cl_git_pass(git_diff_tree_to_tree(g_repo, &opts, c, b, &diff));

	cl_git_pass(git_diff_foreach(
		diff, &exp, diff_file_fn, diff_hunk_fn, diff_line_fn));

	cl_assert(exp.files == 2);
	cl_assert(exp.file_adds == 0);
	cl_assert(exp.file_dels == 0);
	cl_assert(exp.file_mods == 2);

	cl_assert(exp.hunks == 2);

	cl_assert(exp.lines == 8 + 15);
	cl_assert(exp.line_ctxt == 1);
	cl_assert(exp.line_adds == 1);
	cl_assert(exp.line_dels == 7 + 14);

	git_diff_list_free(diff);

	git_tree_free(a);
	git_tree_free(b);
	git_tree_free(c);
}

void test_diff_tree__options(void)
{
	/* grabbed a couple of commit oids from the history of the attr repo */
	const char *a_commit = "6bab5c79cd5140d0";
	const char *b_commit = "605812ab7fe421fdd";
	const char *c_commit = "f5b0af1fb4f5";
	const char *d_commit = "a97cc019851";

	git_tree *a = resolve_commit_oid_to_tree(g_repo, a_commit);
	git_tree *b = resolve_commit_oid_to_tree(g_repo, b_commit);
	git_tree *c = resolve_commit_oid_to_tree(g_repo, c_commit);
	git_tree *d = resolve_commit_oid_to_tree(g_repo, d_commit);

	git_diff_options opts = {0};
	git_diff_list *diff = NULL;
	diff_expects exp;
	int test_ab_or_cd[] = { 0, 0, 0, 0, 1, 1, 1, 1, 1 };
	git_diff_options test_options[] = {
		/* a vs b tests */
		{ GIT_DIFF_NORMAL, 1, 1, NULL, NULL, {0} },
		{ GIT_DIFF_NORMAL, 3, 1, NULL, NULL, {0} },
		{ GIT_DIFF_REVERSE, 2, 1, NULL, NULL, {0} },
		{ GIT_DIFF_FORCE_TEXT, 2, 1, NULL, NULL, {0} },
		/* c vs d tests */
		{ GIT_DIFF_NORMAL, 3, 1, NULL, NULL, {0} },
		{ GIT_DIFF_IGNORE_WHITESPACE, 3, 1, NULL, NULL, {0} },
		{ GIT_DIFF_IGNORE_WHITESPACE_CHANGE, 3, 1, NULL, NULL, {0} },
		{ GIT_DIFF_IGNORE_WHITESPACE_EOL, 3, 1, NULL, NULL, {0} },
		{ GIT_DIFF_IGNORE_WHITESPACE | GIT_DIFF_REVERSE, 1, 1, NULL, NULL, {0} },
	};
	/* to generate these values:
	 * - cd to tests/resources/attr,
	 * - mv .gitted .git
	 * - git diff [options] 6bab5c79cd5140d0 605812ab7fe421fdd
	 * - mv .git .gitted
	 */
	diff_expects test_expects[] = {
		/* a vs b tests */
		{ 5, 3, 0, 2, 4, 0, 0, 51, 2, 46, 3 },
		{ 5, 3, 0, 2, 4, 0, 0, 53, 4, 46, 3 },
		{ 5, 0, 3, 2, 4, 0, 0, 52, 3, 3, 46 },
		{ 5, 3, 0, 2, 5, 0, 0, 54, 3, 48, 3 },
		/* c vs d tests */
		{ 1, 0, 0, 1, 1, 0, 0, 22, 9, 10, 3 },
		{ 1, 0, 0, 1, 1, 0, 0, 19, 12, 7, 0 },
		{ 1, 0, 0, 1, 1, 0, 0, 20, 11, 8, 1 },
		{ 1, 0, 0, 1, 1, 0, 0, 20, 11, 8, 1 },
		{ 1, 0, 0, 1, 1, 0, 0, 18, 11, 0, 7 },
		{ 0 },
	};
	int i;

	cl_assert(a);
	cl_assert(b);

	for (i = 0; test_expects[i].files > 0; i++) {
		memset(&exp, 0, sizeof(exp)); /* clear accumulator */
		opts = test_options[i];

		if (test_ab_or_cd[i] == 0)
			cl_git_pass(git_diff_tree_to_tree(g_repo, &opts, a, b, &diff));
		else
			cl_git_pass(git_diff_tree_to_tree(g_repo, &opts, c, d, &diff));

		cl_git_pass(git_diff_foreach(
			diff, &exp, diff_file_fn, diff_hunk_fn, diff_line_fn));

		cl_assert(exp.files     == test_expects[i].files);
		cl_assert(exp.file_adds == test_expects[i].file_adds);
		cl_assert(exp.file_dels == test_expects[i].file_dels);
		cl_assert(exp.file_mods == test_expects[i].file_mods);
		cl_assert(exp.hunks     == test_expects[i].hunks);
		cl_assert(exp.lines     == test_expects[i].lines);
		cl_assert(exp.line_ctxt == test_expects[i].line_ctxt);
		cl_assert(exp.line_adds == test_expects[i].line_adds);
		cl_assert(exp.line_dels == test_expects[i].line_dels);

		git_diff_list_free(diff);
		diff = NULL;
	}

	git_tree_free(a);
	git_tree_free(b);
	git_tree_free(c);
	git_tree_free(d);
}