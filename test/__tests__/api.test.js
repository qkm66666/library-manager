// api.test.js

import { saveBook, confirmDelete, apiBaseUrl } from '../api';

global.fetch = jest.fn();

jest.mock('../render', () => ({
  renderBooks: jest.fn(),
}));
jest.mock('../pagination', () => ({
  updateStats: jest.fn(),
}));

import { loadBooks } from '../api';

global.fetch = jest.fn();

let booksDatabase = [];
let filteredBooks = [];

describe('loadBooks', () => {
  let booksContainer;

  beforeEach(() => {
    // 设置 DOM 环境
    document.body.innerHTML = `
      <div id="booksContainer"></div>
    `;

    booksContainer = document.getElementById('booksContainer');

    fetch.mockClear();
  });

  it('should fetch and render books', async () => {
    const mockBooks = [{ title: 'Book A', author: 'Author A' }];
    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ data: mockBooks }),
    });

    await loadBooks();

    expect(fetch).toHaveBeenCalled();
  });

  it('should show error message on fetch failure', async () => {
    fetch.mockResolvedValueOnce({
      ok: false,
    });

    await loadBooks();

    expect(booksContainer.innerHTML).toContain('加载失败');
  });

  it('should show error message if fetch throws', async () => {
    fetch.mockRejectedValueOnce(new Error('Network error'));

    await loadBooks();

    expect(booksContainer.innerHTML).toContain('加载失败');
  });
});


describe('saveBook', () => {
  beforeEach(() => {
    fetch.mockClear();
  });

  it('should POST new book when no bookId is provided', async () => {
    const bookData = { title: "Test Book" };
    const mockResponse = { success: true };

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse,
    });

    const result = await saveBook(bookData);

    expect(fetch).toHaveBeenCalledWith(apiBaseUrl, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(bookData),
    });
    expect(result).toEqual(mockResponse);
  });

  it('should PUT existing book when bookId is provided', async () => {
    const bookData = { title: "Updated Book" };
    const bookId = 1;
    const mockResponse = { success: true };

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse,
    });

    const result = await saveBook(bookData, bookId);

    expect(fetch).toHaveBeenCalledWith(`${apiBaseUrl}/${bookId}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(bookData),
    });
    expect(result).toEqual(mockResponse);
  });

  it('should throw error if response not ok', async () => {
    fetch.mockResolvedValueOnce({
      ok: false,
      text: async () => 'Error saving book',
    });

    await expect(saveBook({}, 1)).rejects.toThrow('Error saving book');
  });
});

describe('confirmDelete', () => {
  beforeEach(() => {
    fetch.mockClear();
  });

  it('should call DELETE with correct URL and return true', async () => {
    fetch.mockResolvedValueOnce({ ok: true });

    const result = await confirmDelete(1);

    expect(fetch).toHaveBeenCalledWith(`${apiBaseUrl}/1`, { method: 'DELETE' });
    expect(result).toBe(true);
  });

  it('should throw error if delete fails', async () => {
    fetch.mockResolvedValueOnce({
      ok: false,
      text: async () => 'Delete failed',
    });

    await expect(confirmDelete(1)).rejects.toThrow('Delete failed');
  });
});
