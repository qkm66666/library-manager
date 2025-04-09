export function openAddBookModal() {
    document.getElementById('bookForm').reset();
    document.getElementById('bookId').value = '';
    document.getElementById('modalTitle').textContent = '添加新图书';
    document.getElementById('saveButtonText').textContent = '保存';
    document.getElementById('bookModal').classList.remove('opacity-0', 'invisible');
    document.getElementById('bookModal').classList.add('opacity-100', 'visible');
    document.getElementById('mainContent').classList.add('blur-background');
}

export function editBook(book) {
    document.getElementById('bookId').value = book.id;
    document.getElementById('title').value = book.title;
    document.getElementById('author').value = book.author;
    document.getElementById('isbn').value = book.isbn;
    document.getElementById('category').value = book.category;
    document.getElementById('status').value = book.status;
    document.getElementById('publishDate').value = book.publishDate;
    document.getElementById('description').value = book.description || '';

    document.getElementById('modalTitle').textContent = '编辑图书信息';
    document.getElementById('saveButtonText').textContent = '更新';
    document.getElementById('bookModal').classList.remove('opacity-0', 'invisible');
    document.getElementById('bookModal').classList.add('opacity-100', 'visible');
    document.getElementById('mainContent').classList.add('blur-background');
}

export function closeModal() {
    const modal = document.getElementById('bookModal');
    modal.classList.remove('opacity-100', 'visible');
    modal.classList.add('opacity-0', 'invisible');
    document.getElementById('mainContent').classList.remove('blur-background');
}

export function closeDeleteModal() {
    const modal = document.getElementById('deleteModal');
    modal.classList.remove('opacity-100', 'visible');
    modal.classList.add('opacity-0', 'invisible');
    document.getElementById('mainContent').classList.remove('blur-background');
}