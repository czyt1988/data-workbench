"""
test_dataframe_operate — 数据操作函数测试

覆盖：sort, query, search, describe, pivot_table, eval, filter_by_column_range
正常情况 + 边界情况
"""

import pytest
import pandas as pd
import numpy as np

# Directly import the operate module (bypassing __init__.py which requires da_app runtime)
import importlib.util
import os
_operate_path = os.path.join(
    os.path.dirname(__file__), '..', 'plugins', 'DataAnalysis', 'PyScripts',
    'DADataAnalysis', 'dataframe_operate.py'
)
_spec = importlib.util.spec_from_file_location("dataframe_operate", _operate_path)
_module = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_module)

sort_dataframe = _module.sort_dataframe
query_dataframe = _module.query_dataframe
search_dataframe = _module.search_dataframe
describe_dataframe = _module.describe_dataframe
create_pivot_table = _module.create_pivot_table
eval_expression = _module.eval_expression
filter_by_column_range = _module.filter_by_column_range


@pytest.fixture
def sample_df():
    """创建示例 DataFrame"""
    return pd.DataFrame({
        'name': ['Alice', 'Bob', 'Charlie', 'Diana', 'Eve'],
        'age': [25, 30, 35, 28, 32],
        'score': [85.5, 92.0, 78.5, 88.0, 95.0],
        'city': ['Beijing', 'Shanghai', 'Beijing', 'Guangzhou', 'Shanghai'],
    })


class TestSortDataframe:
    def test_sort_ascending(self, sample_df):
        result = sort_dataframe(sample_df, 'age', ascending=True)
        assert list(result['age']) == [25, 28, 30, 32, 35]

    def test_sort_descending(self, sample_df):
        result = sort_dataframe(sample_df, 'score', ascending=False)
        assert list(result['score']) == [95.0, 92.0, 88.0, 85.5, 78.5]

    def test_sort_multiple_columns(self, sample_df):
        result = sort_dataframe(sample_df, ['city', 'age'])
        assert list(result['city']) == ['Beijing', 'Beijing', 'Guangzhou', 'Shanghai', 'Shanghai']

    def test_sort_does_not_modify_original(self, sample_df):
        original_age = list(sample_df['age'])
        sort_dataframe(sample_df, 'age', ascending=False)
        assert list(sample_df['age']) == original_age


class TestQueryDataframe:
    def test_query_simple(self, sample_df):
        result = query_dataframe(sample_df, 'age > 30')
        assert len(result) == 2
        assert all(result['age'] > 30)

    def test_query_complex(self, sample_df):
        result = query_dataframe(sample_df, "city == 'Beijing' and score > 80")
        assert len(result) == 1
        assert result.iloc[0]['name'] == 'Alice'

    def test_query_empty_result(self, sample_df):
        result = query_dataframe(sample_df, 'age > 100')
        assert len(result) == 0


class TestSearchDataframe:
    def test_search_case_insensitive(self, sample_df):
        result = search_dataframe(sample_df, 'city', 'bei')
        assert len(result) == 2
        assert all(result['city'] == 'Beijing')

    def test_search_case_sensitive(self, sample_df):
        result = search_dataframe(sample_df, 'name', 'alice', case_sensitive=True)
        assert len(result) == 0

    def test_search_regex(self, sample_df):
        result = search_dataframe(sample_df, 'name', '^(A|B)')
        assert len(result) == 2


class TestDescribeDataframe:
    def test_describe_numeric(self, sample_df):
        result = describe_dataframe(sample_df)
        assert 'age' in result.columns
        assert 'score' in result.columns

    def test_describe_all(self, sample_df):
        result = describe_dataframe(sample_df, include='all')
        assert 'name' in result.columns


class TestCreatePivotTable:
    @pytest.fixture
    def pivot_df(self):
        return pd.DataFrame({
            'category': ['A', 'A', 'B', 'B', 'A'],
            'region': ['East', 'West', 'East', 'West', 'East'],
            'sales': [100, 200, 150, 250, 300],
        })

    def test_pivot_table_mean(self, pivot_df):
        result = create_pivot_table(pivot_df, index='category', columns='region', values='sales')
        assert result.loc['A', 'East'] == 200.0  # (100+300)/2

    def test_pivot_table_sum(self, pivot_df):
        result = create_pivot_table(pivot_df, index='category', columns='region', values='sales', aggfunc='sum')
        assert result.loc['A', 'East'] == 400.0  # 100+300


class TestEvalExpression:
    def test_eval_new_column(self, sample_df):
        result = eval_expression(sample_df, 'score_x2 = score * 2')
        assert 'score_x2' in result.columns
        assert list(result['score_x2']) == [171.0, 184.0, 157.0, 176.0, 190.0]

    def test_eval_does_not_modify_original(self, sample_df):
        original_cols = list(sample_df.columns)
        eval_expression(sample_df, 'new_col = age + 1')
        assert list(sample_df.columns) == original_cols


class TestFilterByColumnRange:
    def test_filter_both_bounds(self, sample_df):
        result = filter_by_column_range(sample_df, 'age', min_val=28, max_val=32)
        assert len(result) == 3  # Bob(30), Diana(28), Eve(32)
        assert all(28 <= r <= 32 for r in result['age'])

    def test_filter_min_only(self, sample_df):
        result = filter_by_column_range(sample_df, 'age', min_val=30)
        assert len(result) == 3
        assert all(r >= 30 for r in result['age'])

    def test_filter_max_only(self, sample_df):
        result = filter_by_column_range(sample_df, 'age', max_val=30)
        assert len(result) == 3  # Alice(25), Bob(30), Diana(28)
        assert all(r <= 30 for r in result['age'])

    def test_filter_no_bounds(self, sample_df):
        result = filter_by_column_range(sample_df, 'age')
        assert len(result) == len(sample_df)

    def test_filter_does_not_modify_original(self, sample_df):
        original_len = len(sample_df)
        filter_by_column_range(sample_df, 'age', min_val=999)
        assert len(sample_df) == original_len
