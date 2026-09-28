;(function () {
  'use strict'

  // Figures written with link={imagesdir}/... are anchors around the image.
  // Rather than sending the reader to the file, show it over the page.
  var IMAGE = /\.(svg|png|jpe?g|gif|webp|avif)(\?.*)?$/i

  function targets () {
    var out = []
    var links = document.querySelectorAll('.doc a')
    for (var i = 0; i !== links.length; ++i) {
      var a = links[i]
      var img = a.querySelector('img')
      if (img && a.href && IMAGE.test(a.getAttribute('href'))) out.push(a)
    }
    // A figure written without link= has no anchor, so the image itself is
    // what opens it.
    var images = document.querySelectorAll('.doc .imageblock img, .doc .image img')
    for (var j = 0; j !== images.length; ++j) {
      var bare = images[j]
      if (!bare.closest('a') && IMAGE.test(bare.getAttribute('src'))) out.push(bare)
    }
    return out
  }

  function sourceOf (el) {
    return el.tagName === 'IMG'
      ? el.getAttribute('src')
      : el.getAttribute('href')
  }

  function captionOf (el) {
    var block = el.closest('.imageblock, .image')
    if (block) {
      var title = block.querySelector('.title')
      if (title) return title.textContent.trim()
    }
    var img = el.tagName === 'IMG' ? el : el.querySelector('img')
    return img ? img.getAttribute('alt') || '' : ''
  }

  var box, frame, picture, caption, opener
  var scale = 1          // 1 is the size the figure is shown at on opening
  var fitWidth = 0       // what that size is, in pixels
  var drag = null
  var dragged = false

  // The size the figure opens at: as large as the space allows, less a
  // margin, scaling up as readily as down.
  function fitToSpace () {
    if (!picture.naturalWidth || !picture.naturalHeight) return
    var pad = 32                          // what the lightbox keeps clear
    var room = caption.offsetHeight ? caption.offsetHeight + 16 : 0
    var availWidth = box.clientWidth - pad * 2
    var availHeight = box.clientHeight - pad * 2 - room
    var k = Math.min(
      availWidth / picture.naturalWidth,
      availHeight / picture.naturalHeight
    ) * 0.96                              // a little air around it
    fitWidth = Math.max(1, picture.naturalWidth * k)
    scale = 1
    frame.classList.add('lightbox-fitted')
    frame.classList.remove('lightbox-zoomed')
    picture.style.width = fitWidth + 'px'
    frame.scrollLeft = frame.scrollTop = 0
  }

  // Zoom about a point, so what the reader aimed at stays where it was.
  function zoomTo (next, atX, atY) {
    next = Math.min(Math.max(next, 1), 8)
    if (!fitWidth) fitToSpace()
    var before = picture.getBoundingClientRect()
    var offX = (atX === undefined ? before.left + before.width / 2 : atX) - before.left
    var offY = (atY === undefined ? before.top + before.height / 2 : atY) - before.top
    var ratio = next / scale

    scale = next
    if (scale === 1) {
      picture.style.width = fitWidth + 'px'
      frame.classList.remove('lightbox-zoomed')
      frame.scrollLeft = frame.scrollTop = 0
    } else {
      picture.style.width = fitWidth * scale + 'px'
      frame.classList.add('lightbox-zoomed')
      var after = picture.getBoundingClientRect()
      frame.scrollLeft += offX * ratio - offX + (before.left - after.left)
      frame.scrollTop += offY * ratio - offY + (before.top - after.top)
    }
    picture.style.cursor = scale > 1 ? 'zoom-out' : 'zoom-in'
  }

  function build () {
    box = document.createElement('div')
    box.className = 'lightbox'
    box.setAttribute('role', 'dialog')
    box.setAttribute('aria-modal', 'true')
    box.setAttribute('aria-label', 'Figure')
    box.innerHTML =
      '<button class="lightbox-close" type="button" aria-label="Close">&#215;</button>' +
      '<div class="lightbox-frame"><img alt=""><p class="lightbox-caption"></p></div>'
    frame = box.querySelector('.lightbox-frame')
    picture = box.querySelector('img')
    caption = box.querySelector('.lightbox-caption')

    picture.addEventListener('load', function () {
      if (scale === 1) fitToSpace()
    })
    window.addEventListener('resize', function () {
      if (box.classList.contains('is-open') && scale === 1) fitToSpace()
    })

    box.addEventListener('click', function (e) {
      // A click on the image itself is for looking closer, not for closing.
      if (e.target !== picture && !dragged) close()
      dragged = false
    })

    picture.addEventListener('click', function (e) {
      if (dragged) return
      zoomTo(scale > 1 ? 1 : 2, e.clientX, e.clientY)
    })

    picture.addEventListener('wheel', function (e) {
      e.preventDefault()
      zoomTo(scale * (e.deltaY < 0 ? 1.15 : 1 / 1.15), e.clientX, e.clientY)
    }, { passive: false })

    // Dragging pans, once there is something to pan over.
    picture.addEventListener('pointerdown', function (e) {
      if (scale <= 1) return
      e.preventDefault()
      drag = { x: e.clientX, y: e.clientY, left: frame.scrollLeft, top: frame.scrollTop }
      dragged = false
      picture.setPointerCapture(e.pointerId)
    })
    picture.addEventListener('pointermove', function (e) {
      if (!drag) return
      var dx = e.clientX - drag.x
      var dy = e.clientY - drag.y
      if (Math.abs(dx) + Math.abs(dy) > 3) dragged = true
      frame.scrollLeft = drag.left - dx
      frame.scrollTop = drag.top - dy
    })
    picture.addEventListener('pointerup', function (e) {
      drag = null
      picture.releasePointerCapture(e.pointerId)
    })

    document.body.appendChild(box)
  }

  function open (el) {
    if (!box) build()
    opener = el
    var img = el.tagName === 'IMG' ? el : el.querySelector('img')
    picture.src = sourceOf(el)
    picture.alt = img ? img.getAttribute('alt') || '' : ''
    var text = captionOf(el)
    caption.textContent = text
    caption.style.display = text ? '' : 'none'
    scale = 1
    fitWidth = 0
    picture.style.width = ''
    picture.style.cursor = 'zoom-in'
    frame.classList.remove('lightbox-zoomed', 'lightbox-fitted')
    frame.scrollLeft = frame.scrollTop = 0
    document.body.classList.add('lightbox-open')
    box.classList.add('is-open')
    if (picture.complete) fitToSpace()
    box.querySelector('.lightbox-close').focus()
  }

  function close () {
    if (!box || !box.classList.contains('is-open')) return
    box.classList.remove('is-open')
    document.body.classList.remove('lightbox-open')
    picture.removeAttribute('src')
    if (opener && opener.focus) opener.focus()
    opener = null
  }

  document.addEventListener('keydown', function (e) {
    if (!box || !box.classList.contains('is-open')) return
    if (e.key === 'Escape') close()
    else if (e.key === '+' || e.key === '=') zoomTo(scale * 1.3)
    else if (e.key === '-') zoomTo(scale / 1.3)
    else if (e.key === '0') zoomTo(1)
  })

  document.addEventListener('DOMContentLoaded', function () {
    var list = targets()
    for (var i = 0; i !== list.length; ++i) {
      list[i].classList.add('lightbox-link')
      list[i].addEventListener('click', function (e) {
        // A modified click still opens the file the usual way.
        if (e.metaKey || e.ctrlKey || e.shiftKey || e.altKey || e.button !== 0) return
        e.preventDefault()
        open(this)
      })
    }
  })
})()
