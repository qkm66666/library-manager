let currentPage = 1;
let itemsPerPage = 10;
let filteredBooks = [];

export function renderBooks(books, currentPage, itemsPerPage) {
    const startIndex = (currentPage - 1) * itemsPerPage;
    const endIndex = startIndex + itemsPerPage;
    const booksToShow = books.slice(startIndex, endIndex);

    const booksContainer = document.getElementById('booksContainer');
    booksContainer.innerHTML = '';

    if (booksToShow.length === 0) {
        booksContainer.innerHTML = `
            <div class="col-span-full text-center py-10 text-gray-500">
                <i class="fas fa-book-open text-3xl mb-2"></i>
                <p>没有找到图书</p>
            </div>
        `;
    } else {
        booksToShow.forEach(book => {
            const card = createBookCard(book);
            booksContainer.appendChild(card);
        });
    }
}

export function createBookCard(book) {
    const card = document.createElement('div');
    card.className = 'book-card bg-white rounded-lg shadow overflow-hidden';
    card.innerHTML = `
        <div class="h-48 bg-gradient-to-r from-blue-500 to-blue-600 flex items-center justify-center">
            <i class="fas fa-book-open text-white text-6xl"></i>
        </div>
        <div class="p-6">
            <h3 class="text-xl font-bold truncate">${book.title}</h3>
            <p class="text-gray-600">${book.author}</p>
        </div>
    `;
    return card;
}