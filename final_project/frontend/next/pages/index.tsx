import React from 'react'
import { Component } from 'react'
/*
import Head from 'next/head'
import dynamic from 'next/dynamic'
import {
  Container
} from 'reactstrap'
import Common from '../components/common'
import Nav from '../components/nav'

const ChatNoSSR = dynamic(
  () => import('../components/chat'),
  { ssr: false }
)
*/

class Chat extends Component {
  constructor(props) {
    super(props)
    this.state = {}
  }

  componentDidMount() {
    console.log('component mounted')
  }
/*
  render() {
    return (
      <div>
        <Head>
          <title>Home</title>
          <link rel="icon" href="/favicon.ico" />
        </Head>

        <Common />

        <Nav />

        <Container className="mt-5">
          <h3>Here to help with all your emergency needs</h3>
        </Container>
        <ChatNoSSR />
      </div>
    )
  }
*/
  render() {
    return (
      <p>hello world!</p>
    )
  }
}

export default Chat
